/*
 * Copyright (c) 1999-2017, Parallels International GmbH
 *
 * This file is part of OpenVZ. OpenVZ is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 *
 * Header for timeutils.c, used by ubclogd.
 */

#ifndef _TIMEUTILS_H_
#define _TIMEUTILS_H_

#include <time.h>
#include <sys/time.h>

/* Counts the difference between two timevals.
 * NOTE: it is assumed that t2 > t1.
 * Result is returned in struct pointed by tdiff.
 */
void timediff(const struct timeval *t1, const struct timeval *t2,
		struct timeval *tdiff);


/* Decreases *st by value of *t. Normal return code is 0.
 * If resulting value is negative, *st is set to 0, and -1 is returned.
 */
int decrease_sleeptime(struct timespec *st, const struct timeval *t);

#endif /* _TIMEUTILS_H_ */
