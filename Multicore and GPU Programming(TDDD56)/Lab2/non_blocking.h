/*
 * non_blocking.h
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

#include <stddef.h>

#ifndef NON_BLOCKING_H_
#define NON_BLOCKING_H_

/*
Both cat() and software_cas() take as a first argument the address of the
pointer to modify (a pointer to pointer), then its old value (where is
points to) and its new value (where it should point to).
software_cas takes an additional lock to simulate the atomic swap operation

WARNING: These implementations do not implement a full "swap" operation:
a successful cas() operation copies the new value to *reg, but NOT new to
*reg.
*/
#if NON_BLOCKING == 1
size_t cas(size_t* reg, size_t old, size_t new);
#elif NON_BLOCKING == 2
#include <pthread.h>
size_t software_cas(size_t * reg, size_t old, size_t new, pthread_mutex_t* lock);
#endif

#endif /* NON_BLOCKING_H_ */
