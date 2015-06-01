/* $Id$
 *
 * Copyright (C) SWsoft, 1999-2007. All rights reserved.
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
