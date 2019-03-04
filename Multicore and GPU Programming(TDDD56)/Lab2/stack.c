/*
 * stack.c
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
 *     but WITHOUT ANY WARRANTY without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with TDDD56. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef DEBUG
#define NDEBUG
#endif

#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "stack.h"
#include "non_blocking.h"

#if NON_BLOCKING == 0
#warning Stacks are synchronized through locks
#else
#if NON_BLOCKING == 1
#warning Stacks are synchronized through hardware CAS
#else
#warning Stacks are synchronized through lock-based CAS
#endif
#endif

int
stack_check(stack_t *head)
{
// Do not perform any sanity check if performance is bein measured
#if MEASURE == 0
	// This test fails if the task is not allocated or if the allocation failed
	assert(head != NULL);
#elif MEASURE == 2
	// Test that the data should increase only 1 between this and previous block
	if (head->next != NULL){
		assert(head->data == head->next->data + 1);
	}
#endif
	return 1;
}

int
stack_push(stack_t **head,stack_t *reg)
{
	stack_t *old;
#if NON_BLOCKING == 0
  // Implement a lock_based stack
  pthread_mutex_lock(&stack_lock);
    if ((*head) == NULL) {
	    reg->data = 0;
    }
    else {
        reg->data = (*head)->data + 1;
    }
	reg->next = (*head);
	(*head) = reg;
  pthread_mutex_unlock(&stack_lock);
#elif NON_BLOCKING == 1
	// Implement a hardware CAS-based stack
	do{
		old = (*head);
        if ((*head) == NULL) {
	        reg->data = 0;
        }
        else {
            reg->data = old->data + 1;
        }
		reg->next = old;
	}while(cas((size_t*)head, (size_t)old, (size_t)reg) != (size_t)old);
#else
  // Implement a software CAS-based stack
	do{
		old = (*head);
        if ((*head) == NULL) {
	        reg->data = 0;
        }
        else {
            reg->data = old->data + 1;
        }
		reg->next = old;
	}while(software_cas((size_t*)head, (size_t)old, (size_t)reg, &stack_lock) != (size_t)old);
#endif
  stack_check((stack_t*)(*head));

  return 0;
}

stack_t *
stack_pop(stack_t **head)
{
	stack_t *reg;
	stack_t *old;
#if NON_BLOCKING == 0
  // Implement a lock_based stack
  pthread_mutex_lock(&stack_lock);
	old = (*head);
	if (old == NULL){
	    return 0;
	}
	reg = old->next;
	(*head) = reg;
  pthread_mutex_unlock(&stack_lock);
#elif NON_BLOCKING == 1
  // Implement a harware CAS-based stack
	do {
		old = (*head);
		if (old == NULL){
			return 0;
		}
		reg = old->next;
	}while(cas((size_t*)head, (size_t)old, (size_t)reg) != (size_t)old);
#else
  // Implement a software CAS-based stack
	do {
		old = (*head);
		if (old == NULL){
			return 0;
		}
		reg = old->next;
	}while(software_cas((size_t*)head, (size_t)old, (size_t)reg, &stack_lock) != (size_t)old);
#endif

  return old;
}
