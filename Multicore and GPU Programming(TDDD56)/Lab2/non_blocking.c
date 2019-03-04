/*
 * non_blocking.c
 *
 *  Created on: 19 Oct 2011
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

#ifndef DEBUG
#define NDEBUG
#endif

#include <stddef.h>
#include "non_blocking.h"

#if NON_BLOCKING == 1

#if defined i386 || __i386__ || __i486__ || __i586__ || __i686__ || __i386 || __IA32__ || _M_IX86 || _M_IX86 || __X86__ || _X86_ || __THW_INTEL__ || __I86__ || __INTEL__ || __x86_64 || __x86_64__

inline size_t
cas(size_t* reg, size_t oldval, size_t newval)
{
  asm volatile( "lock; cmpxchg %2, %1":
                "=a"(oldval):
                "m"(*reg), "r"(newval), "a"(oldval):
                "memory" );

  return oldval;
}

#elif defined __GNUC__

inline size_t
cas(size_t* reg, size_t oldval, size_t newval)
{
#warning Using GCC Compare-and-swap
  return __sync_val_compare_and_swap(reg, oldval, newval);
}

#else

#warning Unsupported compiler and architecture; no CAS available
inline size_t
cas(size_t* reg, size_t oldval, size_t newval)
{
  return 0;
}
#endif

#else
#include <string.h>
#include <pthread.h>

inline size_t
software_cas(size_t* reg, size_t oldval, size_t newval, pthread_mutex_t *lock)
{
  pthread_mutex_lock(lock);
  size_t old_local = *reg;
  if(old_local == oldval)
    {
      memcpy(reg, &newval, sizeof(void*));
      pthread_mutex_unlock(lock);

      return old_local;
    }
  else
    {
      pthread_mutex_unlock(lock);
      return old_local;
    }
}
#endif


