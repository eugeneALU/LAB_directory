/*
 * test.c
 *
 *  Created on: 18 Oct 2011
 *  Copyright 2011 Nicolas Melot
 *
 * This file is part of TDDD56.
 *
 *     TDDD56 is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     TDDD56 is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with TDDD56. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stddef.h>
#include <unistd.h>

#include "test.h"
#include "stack.h"
#include "non_blocking.h"

//define multi-line (add / at the end of line)
#define test_run(test)\
  printf("[%s:%s:%i] Running test '%s'... ", __FILE__, __FUNCTION__, __LINE__, #test);\
  if(test())\
  {\
    printf("passed\n");\
  }\
  else\
  {\
    printf("failed\n");\
  }

/* Helper function for measurement */
double timediff(struct timespec *begin, struct timespec *end)
{
	double sec = 0.0, nsec = 0.0;
   if ((end->tv_nsec - begin->tv_nsec) < 0)
   {
      sec  = (double)(end->tv_sec  - begin->tv_sec  - 1);
      nsec = (double)(end->tv_nsec - begin->tv_nsec + 1000000000);
   } else
   {
      sec  = (double)(end->tv_sec  - begin->tv_sec );
      nsec = (double)(end->tv_nsec - begin->tv_nsec);
   }
   return sec + nsec / 1E9;
}

typedef int data_t;
#define DATA_SIZE sizeof(data_t)
#define DATA_VALUE 0

#ifndef NDEBUG
int
assert_fun(int expr, const char *str, const char *file, const char* function, size_t line)
{
	if(!(expr))
	{
		fprintf(stderr, "[%s:%s:%zu][ERROR] Assertion failure: %s\n", file, function, line, str);
		abort();
		// If some hack disables abort above
		return 0;
	}
	else
		return 1;
}
#endif

stack_t *stack;
data_t data;
//lock for lock-based stack and software_cas
pthread_mutexattr_t stack_mutex_attr;
pthread_mutex_t stack_lock;

#if MEASURE != 0
struct stack_measure_arg
{
  int id;
  stack_t pool[MAX_PUSH_POP / NB_THREADS];
};
typedef struct stack_measure_arg stack_measure_arg_t;

struct timespec t_start[NB_THREADS], t_stop[NB_THREADS], start, stop;

#if MEASURE == 1
void*
stack_measure_pop(void* arg)
{
  stack_measure_arg_t *args = (stack_measure_arg_t*) arg;
  int i;

  clock_gettime(CLOCK_MONOTONIC, &t_start[args->id]);
  for (i = 0; i < MAX_PUSH_POP / NB_THREADS; i++)
  {
    // See how fast your implementation can pop MAX_PUSH_POP elements in parallel
    stack_pop(&stack);
  }
  clock_gettime(CLOCK_MONOTONIC, &t_stop[args->id]);
  /*pthread_mutex_lock(&stack_lock);            //print out the last pointer of each thread to see pop procedure is correct
  printf("END POP POINTER = %p\n",stack);
  printf("END COUNT = %d\n",stack->data);
  pthread_mutex_unlock(&stack_lock);*/
  return NULL;
}
#elif MEASURE == 2
void*
stack_measure_push(void* arg)
{
  stack_measure_arg_t *args = (stack_measure_arg_t*) arg;
  int i;

  clock_gettime(CLOCK_MONOTONIC, &t_start[args->id]);
  for (i = 0; i < MAX_PUSH_POP / NB_THREADS; i++)
  {
    // See how fast your implementation can push MAX_PUSH_POP elements in parallel
    stack_push(&stack, &args->pool[i]);
  }
  clock_gettime(CLOCK_MONOTONIC, &t_stop[args->id]);

  return NULL;
}
#endif
#endif

void
test_init()
{
  // Initialize your test batch
}

void
test_setup()
{
  // Allocate and initialize your test stack before each test
  data = DATA_VALUE;

  // Allocate a new stack and reset its values
  stack = malloc(sizeof(stack_t));
#if MEASURE == 1
  int i;
  //push MAX_PUSH_POP element in the stack for pop test
  printf("Creating Test stack...\n");
  printf("START POINTER = %p\n", stack);
  stack->next = NULL;
  stack->data = 0;
  for (i = 0; i < MAX_PUSH_POP; i++)
  {
    stack_t *tmp = malloc(sizeof(stack_t));
    tmp->data = stack->data + 1;
    tmp->next = stack;
    stack = tmp;
  }
  printf("Finished!!\n");
#else
  // Reset explicitely all members to a well-known initial value
  stack->next = NULL;
  stack->data = data;
#endif
  return 0;
}

void
test_teardown()
{
  stack_t *tmp;
  while(stack->next != NULL){
    tmp = stack;
    stack = stack->next;
    free(tmp);
  }
}

void
test_finalize()
{
  // Destroy stack
  if (stack != NULL){
    free(stack);
  }
}

/* structure that used to passing parameter in stack_push */
struct test_arg
{
    stack_t *reg;
    stack_t **stack;
};
typedef struct test_arg test_arg_t;

/*
 * Wrapper function just for easing the parameter passing procedure in pthread
 */
int
stack_push_wrapper(void* param)
{
    test_arg_t *parameter = (test_arg_t*) param;
    stack_push(parameter->stack, parameter->reg);
    return 0;
}

/* CAUTION: create globally, so that test_pop_safe and test_push_safe can use the same stack */
stack_t pool_for_test[NB_THREADS];           //create a pool for push test
int
test_push_safe()
{
  // TEST:
  // Make sure your stack remains in a good state with expected content when
  // several threads push concurrently to it
  pthread_attr_t attr;
  pthread_t thread[NB_THREADS];
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  test_arg_t param[NB_THREADS];       //parameter that will pass to stack_push
  int i;
  for (i = 0; i < NB_THREADS; i++)
  {
    param[i].reg = &pool_for_test[i];
    param[i].stack = &stack;
    pthread_create(&thread[i], &attr, &stack_push_wrapper, (void*)&param[i]);
    // check if the stack is in a consistent state
    assert(stack_check(stack));
  }
  for (i = 0; i < NB_THREADS; i++)
  {
    pthread_join(thread[i], NULL);
  }
  // check if the stack is in a consistent state
  int res = assert(stack->data == NB_THREADS);

  return res && assert(stack->next != NULL);
}

int
test_pop_safe()
{
  // TEST:
  // Make sure your stack remains in a good state with expected content when
  // several threads pop concurrently to it
  pthread_attr_t attr;
  pthread_t thread[NB_THREADS];
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  int i;
  for (i = 0; i < NB_THREADS; i++)
  {
    pthread_create(&thread[i], &attr, &stack_pop, (void*)&stack);
    // check if the stack is in a consistent state
    assert(stack_check(stack));
  }
  for (i = 0; i < NB_THREADS; i++)
  {
    pthread_join(thread[i], NULL);
  }
  // same NB_THREADS in push test, so the stack pointer should back to first one
  // which doesn't have next element
  return assert(stack->data == 0) && assert(stack->next == NULL);
}

// 3 Threads should be enough to raise and detect the ABA problem
#define ABA_NB_THREADS 3

#if NON_BLOCKING != 0

int
aba_pop(stack_t **head){
  stack_t *reg;
  stack_t *old;
  do {
		old = *head;
    reg = old->next;
		if (reg == NULL){
			return 0;
		}
    sleep(1);   //force interrupt
#if NON_BLOCKING == 1
	}while (cas((size_t*)head, (size_t)old, (size_t)reg) != (size_t)old);
#elif NON_BLOCKING ==2
  }while (software_cas((size_t*)head, (size_t)old, (size_t)reg,  &stack_lock) != (size_t)old);
#endif
  return 0;
}
#endif

int
test_aba()
{
#if NON_BLOCKING == 1 || NON_BLOCKING == 2
  test_arg_t param;
  int success=0, aba_detected = 0;
  int trytime = 0;
  int i;
  stack_t A, B, C;
  A.data = 3;
  B.data = 2;
  C.data = 1;
  pthread_attr_t attr;
  pthread_t thread[ABA_NB_THREADS];

  param.reg = &A;
  param.stack = &stack;
  while(!success){
    trytime++;
    A.next = &B;
    B.next = &C;
    C.next = NULL;
    stack = &A;

    pthread_attr_init(&attr);
    pthread_create(&thread[0], &attr, &aba_pop,(void*)&stack);  //pop A, but interupt(assume) also can use {stack_pop} which needs more try to trigger aba problem
                                                                // can observe through variable{trytime}
    pthread_create(&thread[1], &attr, &stack_pop,(void*)&stack);  //pop A
    pthread_create(&thread[2], &attr, &stack_pop,(void*)&stack);  //pop B
    pthread_join(thread[1], NULL);      //make sure thread finished its work
    pthread_create(&thread[1], &attr, &stack_push_wrapper,(void*)&param); //push A

    for (i = 0; i < ABA_NB_THREADS; i++)
    {
      pthread_join(thread[i], NULL);
    }
    if (stack != NULL){
      aba_detected = assert(stack->data == 2); //==2 means it points to B, but B has already been popped
    }
    success = aba_detected;
  }
  printf("\nTRY TIME = %d\n", trytime);
  return success;
#else
  // No ABA is possible with lock-based synchronization. Let the test succeed only
  return 1;
#endif
}

// TEST:
// CAS function
struct thread_test_cas_args
{
  int id;
  size_t* counter;
  pthread_mutex_t *lock;
};
typedef struct thread_test_cas_args thread_test_cas_args_t;

void*
thread_test_cas(void* arg)
{
#if NON_BLOCKING != 0
  thread_test_cas_args_t *args = (thread_test_cas_args_t*) arg;
  int i;
  size_t old, local;

  for (i = 0; i < MAX_PUSH_POP; i++)
    {
      do {
        old = *args->counter;
        local = old + 1;
#if NON_BLOCKING == 1
      } while (cas(args->counter, old, local) != old);
#elif NON_BLOCKING == 2
      } while (software_cas(args->counter, old, local, args->lock) != old);
#endif
    }
#endif

  return NULL;
}

// Make sure Compare-and-swap works as expected
int
test_cas()
{
#if NON_BLOCKING == 1 || NON_BLOCKING == 2
  pthread_attr_t attr;
  pthread_t thread[NB_THREADS];
  thread_test_cas_args_t args[NB_THREADS];
  pthread_mutexattr_t mutex_attr;
  pthread_mutex_t lock;
  size_t counter;

  int i, success;

  counter = 0;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  pthread_mutexattr_init(&mutex_attr);
  pthread_mutex_init(&lock, &mutex_attr);

  for (i = 0; i < NB_THREADS; i++)
  {
    args[i].id = i;
    args[i].counter = &counter;
    args[i].lock = &lock;
    pthread_create(&thread[i], &attr, &thread_test_cas, (void*) &args[i]);
  }

  for (i = 0; i < NB_THREADS; i++)
  {
    pthread_join(thread[i], NULL);
  }

  success = assert(counter == (size_t)(NB_THREADS * MAX_PUSH_POP));

  if (!success)
  {
    printf("Got %ti, expected %i. ", counter, NB_THREADS * MAX_PUSH_POP);
  }

  return success;
#else
  return 1;
#endif
}

int
main(int argc, char **argv)
{
  int i;
  setbuf(stdout, NULL);
  pthread_mutexattr_init(&stack_mutex_attr);
  pthread_mutex_init(&stack_lock, &stack_mutex_attr);

  test_setup();

// MEASURE == 0 -> run unit tests
#if MEASURE == 0
  test_init();

  test_run(test_cas);

  test_run(test_push_safe);
  test_run(test_pop_safe);
  test_run(test_aba);

#else
  pthread_t thread[NB_THREADS];
  pthread_attr_t attr;
  stack_measure_arg_t arg[NB_THREADS];
  pthread_attr_init(&attr);

  clock_gettime(CLOCK_MONOTONIC, &start);
  for (i = 0; i < NB_THREADS; i++)
    {
      arg[i].id = i;
#if MEASURE == 1
      pthread_create(&thread[i], &attr, stack_measure_pop, (void*)&arg[i]);
#else
      pthread_create(&thread[i], &attr, stack_measure_push, (void*)&arg[i]);
#endif
    }

  for (i = 0; i < NB_THREADS; i++)
  {
    pthread_join(thread[i], NULL);
  }
  clock_gettime(CLOCK_MONOTONIC, &stop);

  // Print out results
  for (i = 0; i < NB_THREADS; i++)
  {
        printf("Thread %d time: %f\n", i, timediff(&t_start[i], &t_stop[i]));
  }
  printf("Total time: %f\n", timediff(&start, &stop));

#if MEASURE == 2                //just used to check that the push procedure is correct (be careful of how many times we push {MAX_PUSH_POP/NB_THREADS} rounds to integer)
  printf("start check\n");
  int c = 0;
  while(stack->next != NULL){
    c++;
    stack = stack->next;
  }
  printf("COUNT = %d\n", c);
#endif

#endif

  return 0;
}
