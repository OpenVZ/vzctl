/*
 * Copyright (c) 1999-2017, Parallels International GmbH
 * Copyright (c) 2017-2019 Virtuozzo International GmbH. All rights reserved.
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
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 *
 * Various time-related functions for ubclogd. See description in header.
 */

#include <time.h>
#include <sys/time.h>
#include <errno.h>

void timediff(const struct timeval *t1, const struct timeval *t2,
		struct timeval *tdiff)
{
	tdiff->tv_sec = t2->tv_sec - t1->tv_sec;
	tdiff->tv_usec = t2->tv_usec - t1->tv_usec;
	if (tdiff->tv_usec < 0)
	{
		tdiff->tv_sec--;
		tdiff->tv_usec = 1000000 + tdiff->tv_usec;
	}
}

int decrease_sleeptime(struct timespec *st, const struct timeval *t)
{
	st->tv_sec -= t->tv_sec;
	if (t->tv_usec != 0)
	{
		st->tv_nsec -= t->tv_usec * 1000; /* nsec = 1000 * usec */
	}
	if (st->tv_nsec < 0)
	{
		st->tv_sec--;
		st->tv_nsec = 1000000000L + st->tv_nsec;
	}
	if (st->tv_sec < 0)
	{
		st->tv_sec = 0; st->tv_nsec = 0;
		return -1;
	}
	return 0;
}
