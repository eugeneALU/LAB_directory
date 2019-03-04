/*
 * stack.h
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

#ifndef TEST
#define TEST

// If a default assert is already defined, undefine it first
#ifdef assert
#undef assert
#endif

#ifdef NDEBUG
#undef NDEBUG
#endif

#if DEBUG == 0
#define NDEBUG
#endif

// Enable assert() only if NDEBUG is not set
#ifndef NDEBUG
int assert_fun(int expr, const char *str, const char *file, const char* function, size_t line);
#define assert(expr) assert_fun(expr, #expr, __FILE__, __FUNCTION__, __LINE__);
#else
// Otherwise define it as just running the expression
#define assert(expr) expr
#endif

#endif
