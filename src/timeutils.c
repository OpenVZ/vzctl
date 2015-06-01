/* $Id$
 *
 * Copyright (C) SWsoft, 1999-2007. All rights reserved.
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
