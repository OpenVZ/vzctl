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
 * A small daemon to periodically log some selected UBC parameters.
 * List of UBC parameters is compiled in.
 *
 * Command-line options are:
 *  -i <seconds>	update interval
 *  -r <kilobytes>	amount of disk space to reserve for logs
 *  -o <logfile>	name of the log file to write to
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <signal.h>

#include "ubclogd.h"
#include "timeutils.h"

/* Uncomment the below line to log milliseconds together with time */
/* #define NEED_MILLISECONDS */

unsigned int update_interval = UPDATE_INTERVAL;
long reserved_space = RESERVED_SPACE;
char * log_file = LOG_FILE;
int of = -1;
int fl_reserve = 1; /* reserve free space */

/* flag is set by sighandler if we want to truncate the file */
int fl_truncate = 0;
/* flag is set by sighandler if we want to reopen the file */
int fl_reopen = 0;

struct sigaction sighup;
struct sigaction sigusr1;

void usage(void)
{
	fprintf(stderr,
FULL_NAME ".  Copyright (C) SWsoft, 1999-2007. All rights reserved.\n\n"
"Usage: " PROGRAM_NAME " [options]\n"
"Available options are:\n"
"  -i - periodical log interval, in seconds (default is %d)\n"
"  -r - space to reserve in log file, in Kb (default is %d Kb)\n"
"  -o - name of file to write log to (default is \"" LOG_FILE "\")\n"
"  -n - do not fork into background\n"
"  -h - print this help text\n"
, UPDATE_INTERVAL, RESERVED_SPACE);
}

/* This function expands file to contain at least
 * BLOCKSIZE*space bytes containing FILL_CHAR
 * at the end of file.
 * FIXME: use add_blocks here!
 */
int prep_file(int fd, long space)
{
	char buf[BLOCKSIZE];
	off_t curpos;
	long i;
	ssize_t w;
	size_t wc;
	long free_blocks;
	int found;
	int pos;
	int r;

	if (reserved_space == 0)
		return 0;

	curpos = lseek(fd, 0, SEEK_END);
	if (curpos == (off_t)-1)
	{
		fprintf(stderr, "Can't lseek in file: %s\n",
				strerror(errno));
		return -1;
	}

	/* Find out how many extra space do we have at end-of-file */
	found = 0; free_blocks = 0; pos = 0;
	while (!found && ((free_blocks + 1) * BLOCKSIZE <= curpos))
	{
		lseek(fd, (free_blocks + 1) * -BLOCKSIZE, SEEK_END);
		r = read(fd, buf, BLOCKSIZE);
		if (r != BLOCKSIZE)
		{
			fprintf(stderr, "Read %d bytes: %s\n",
					r, strerror(errno));
			return 0;
		}
		for (pos = BLOCKSIZE - 1; pos > 0 && buf[pos] == FILL_CHAR;
				pos--);
		if (pos == 0)
			free_blocks++;
		else
			found = 1;
		/*
		printf("read %d\toffset %d\tfree_blocks %d\n", r,
				(free_blocks + 1) * -BLOCKSIZE,
				free_blocks); */
	}
/*	printf("Found free blocks: %d\tbytes: %d\n", free_blocks, pos);
	if (free_blocks >= space)
	{
		fprintf(stderr, "%d: Setting position to %d\n", __LINE__,
			lseek(fd, ((free_blocks + 1) * -BLOCKSIZE) +
				pos + 1, SEEK_END));
		return 0;
	}
*/
	/* Add some more free blocks */
	space -= free_blocks;
/*	printf("Adding %d blocks\n", space); */
	lseek(fd, 0, SEEK_END);
	memset(buf, FILL_CHAR, BLOCKSIZE);
	wc = BLOCKSIZE;
	for(i = 0; i < space; i++)
	{
		w = write(fd, buf, wc);
		if (w != wc)
		{
			if (errno == EINTR)
			{
				/* repeat the write */
				wc = wc - w;
				i--;
			}
			else
			{
				fprintf(stderr, "Can't write to file: "
						"%s\n", strerror(errno));
				return -2;
			}
		}
		else
			wc = BLOCKSIZE;
	}
/*
	fprintf(stderr, "free_blocks = %d\nspace = %d\npos = %d\n",
		free_blocks, space, pos);
	fprintf(stderr, "%d: Setting position to %d\n", __LINE__,
*/
	lseek(fd, (free_blocks + space + (pos != 0)) *
		-BLOCKSIZE + pos + (pos != 0), SEEK_END);
	return 0;
}

/* Adds blocks of free space to the end of file
 * Parameters:
 *  fd		file descriptor
 *  num		number of blocks to be added
 * Returns:
 *   0		O.K.
 *  -1		write() returned an error other than EAGAIN
 *
 * Side note: we can not use truncate() here because we will have a file
 * with a gap in it, but we need actual blocks to be allocated in FS
 */
int add_blocks(int fd, int num)
{
	char buf[BLOCKSIZE];
	off_t pos;
	ssize_t w;
	size_t wc;
	int i;

	if (reserved_space == 0)
		return 0;

	pos = lseek(fd, 0, SEEK_CUR); /* Save current position */
	lseek(fd, 0, SEEK_END);
	memset(buf, FILL_CHAR, BLOCKSIZE);
	wc = BLOCKSIZE;
	for(i = 0; i < num; i++)
	{
		w = write(fd, buf, wc);
		if (w != wc)
		{
			if (errno == EINTR)
			{
				/* repeat the write */
				wc = wc - w;
				i--;
			}
			else
			{
				fprintf(stderr, "Can't write to file: "
						"%s\n", strerror(errno));
				return -2;
			}
		}
		else
			wc = BLOCKSIZE;
	}
	lseek(fd, pos, SEEK_SET);
	return 0;
}

/* Truncates the of file to its current position, and sets fl_reserve to 0 */
int truncate_file(void)
{
	off_t cur;
	cur = lseek(of, 0, SEEK_CUR);
	if (cur < 0)
		return -1;
	return ftruncate(of, cur);
}

/* Tries to reopen the file and reserve free space.
 * Returns -1 if some operation failed.
 * Uses global variables: of, log_file, fl_reserve, reserved_space.
 */
int reopen_file(void)
{
	if (of <= 0)
		close(of);
	if ((of = open(log_file, O_RDWR | O_CREAT,
		S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0)
		return -1;
	if (flock(of, LOCK_EX | LOCK_NB) < 0)
		return -1;
	if (fl_reserve)
		return prep_file(of, reserved_space);
	return 0;
}

/* Handler for SIGUSR1
 * Used before log rotating.
 */
void sigusr1h(int x)
{
	fl_truncate = 1;
	fl_reserve = 0;
}

/* Handler for SIGHUP
 * Used after log rotating.
 */
void sighuph(int x)
{
	fl_reserve = 1;
	fl_reopen = 1;
}

long log_ubc(int of)
{
	FILE * f;
	char buf[1024];
	char iobuf[1024];
	char wbuf[4096];
	char * wi;
	int w, ws;
	unsigned long veid;
	const char* fmt;
	char param[1024];
	unsigned long values[5];
	int i;
	int print_veid = 0;
	long written = 0;
	int indent;

	f = fopen(UBC_FILE, "r");
	if (f == NULL)
	{
		/* Log the error */
		snprintf(buf, sizeof(buf), "Error opening " UBC_FILE ": %s\n",
			strerror(errno));
		i = strlen(buf);
		write(of, buf, i);
		return i;
	}
	setvbuf(f, iobuf, _IOLBF, sizeof(iobuf));
	wi = wbuf;
	ws = sizeof(wbuf);
	while (fgets(buf, sizeof(buf), f) != NULL)
	{
		if (sscanf(buf, "%lu", &veid) >= 1)
		{
			/* we got VEID */
			fmt = "%*lu:%s%lu%lu%lu%lu%lu";
			print_veid = 1;
			indent = 1;
		}
		else
		{
			fmt = "%s%lu%lu%lu%lu%lu";
			indent = 12;
		}
		if (sscanf(buf, fmt, param, values,
			values + 1, values + 2, values + 3, values + 4) >= 5)
		{
			if (print_veid)
			{
				/* Do we need to write prev. buffer? */
				if (ws < sizeof(wbuf))
				{
					write(of, wbuf, sizeof(wbuf) - ws);
					written += sizeof(wbuf) - ws;
					ws = sizeof(wbuf);
					wi = wbuf;
				}
				/* Print veid */
				w = sprintf(wbuf, "%10lu:", veid);
				if (w > 0)
				{
					wi += w;
					ws -= w;
				}
				print_veid = 0;
			}
			for(i = 0; ubc_to_log[i] != NULL; i++)
				if (strcmp(ubc_to_log[i], param) == 0)
				{
					/* log it */
					w = snprintf(wi, ws, "%*s  %-12s "
						"%10lu %10lu %10lu %10lu "
						"%10lu\n",
						indent,	"", param,
						values[0], values[1],
						values[2], values[3],
						values[4]);
					if (w > 0)
					{
						wi += w;
						ws -= w;
					}
				}
		}
	}
	/* Write buffer */
	if (ws < sizeof(wbuf))
		write(of, wbuf, sizeof(wbuf) - ws);
	written += sizeof(wbuf) - ws;
	/* Close /proc/ubc */
	fclose(f);
	return written;
}

long do_log(int f)
{
	char buf[1024];
	int ts;
#ifdef NEED_MILLISECONDS
	struct timeval tv;
#endif

/*	ssize_t w = 0; */
	time_t t;
	struct tm * tm;
	long written = 0;
/*
	int pos;
	pos = lseek(f, 0, SEEK_CUR);
	fprintf(stderr, "Current position is %d\n", pos);
*/
	t = time(NULL);
	tm = localtime(&t);
	ts = strftime(buf, sizeof(buf), "-- %b %e %H:%M:%S %z --\n", tm);
#ifdef NEED_MILLISECONDS
	gettimeofday(&tv, NULL);
	ts += snprintf(buf + ts, sizeof(buf) - ts, "%ld ms\n", tv.tv_usec);
#endif
	written += write(f, buf, ts);
	written += log_ubc(f);
/*
	pos = lseek(f, 0, SEEK_CUR);
	fprintf(stderr, "Current position is %d\n", pos);
*/
	return written;
}

int sleep_between(int seconds, long nsec_adjust,
	const struct timeval *tx, const struct timeval *ty)
{
	struct timeval td, t1, t2;
	struct timespec st;
	timediff(tx, ty, &td);
	st.tv_sec = seconds; st.tv_nsec = 0;
	if (decrease_sleeptime(&st, &td) > 0)
		return -1; /* No time to sleep */
	st.tv_nsec -= nsec_adjust;
	if (st.tv_nsec < 0)
	{
		st.tv_sec -= 1;
		st.tv_nsec += 1000000000;
	}
	else if (st.tv_nsec > 999999999)
	{
		st.tv_sec += 1;
		st.tv_nsec -= 1000000000;
	}
	while (nanosleep(&st, &st) != 0)
	{
		gettimeofday(&t1, NULL);
		if (errno == EINVAL)
			return -2; /* invalid argument - should never happen */

		/* Interrupted - check the flags */
		if (fl_truncate != 0)
		{
			truncate_file();
			fl_truncate = 0;
		}
		if (fl_reopen != 0)
		{
			reopen_file();
			fl_reopen = 0;
		}
		gettimeofday(&t2, NULL);
		timediff(&t1, &t2, &td);
		if (decrease_sleeptime(&st, &td) > 0)
			return -1; /* No time to sleep */
	}
	return 0;
}

static void start_log(int f)
{
	struct timeval t1, t2, t3;
	long written = 0;
	int written_blocks = 0;
	long adj_nsec = 0;

	while(1)
	{
		gettimeofday(&t1, NULL);
		if (fl_reserve)
		{
			written += do_log(f);
			written_blocks = written / BLOCKSIZE;
			if (written_blocks > BLOCK_THRESHOLD)
			{
				if (add_blocks(f,
					written_blocks + BLOCK_EXTRA) == 0)
				{
					written -= BLOCKSIZE *
						(written_blocks + BLOCK_EXTRA);
				}
			}
		}
		else
		{
			do_log(f);
		}
		gettimeofday(&t2, NULL);
		sleep_between(update_interval, adj_nsec, &t1, &t2);
		gettimeofday(&t3, NULL);
		adj_nsec += (t3.tv_usec - t1.tv_usec) * 1000;
	}
}

void reserve_memory(void)
{
	FILE * f;
	/* Get some stack memory */
	char reserved[16384];
	/* Touch it */
	memset(reserved, 1, sizeof(reserved));
	/* Write to file - this also allocates some buffer (???) */
	write(of, STARTUP_MSG, sizeof(STARTUP_MSG) - 1);
	write(of, UBC_HEADER, sizeof(UBC_HEADER) - 1);
	f = fopen(UBC_FILE, "r");
	/* Lock it */
	mlockall(MCL_CURRENT | MCL_FUTURE);
	fclose(f);
}

void daemonize(void)
{
	pid_t p = fork();
	if (p < 0)
	{
		fprintf(stderr, "fork() failed: %s\n", strerror(errno));
		exit(1);
	}
	if (p == 0) /* child */
	{
		/* Init time zone */
		tzset();
		/* daemonize */
		setsid();
		chdir("/");
		close(0);
		close(1);
		close(2);
		/* Allocate enough stack */
		reserve_memory();
	}
	else /* parent */
	{
		exit(0);
	}
}

int main(int argc, char ** argv)
{
	int opt;
	char * endptr;
	FILE * ub;
	int daemon = 1; /* Should we daemonize */

	if (geteuid() != 0)
	{
		fprintf(stderr, "ERROR: Only root can execute this program.\n");

		return 1;
	}
	/* Process command-line options */
	while ((opt = getopt(argc, argv, "i:r:o:nh")) > 0)
	{
		switch(opt)
		{
		   case 'i':
			update_interval = strtol(optarg, &endptr, 10);
			if ((*endptr != '\0') || (update_interval <= 0))
			{
				fprintf(stderr, "Invalid argument "
					"for -i: %s\n", optarg);
				return 1;
			}
			break;
		   case 'r':
			reserved_space = strtol(optarg, &endptr, 10);
			if ((*endptr != '\0') || (reserved_space < 0))
			{
				fprintf(stderr, "Invalid argument "
					"for -r: %s\n", optarg);
				return 1;
			}
			break;
		   case 'o':
			log_file = strdup(optarg);
			break;
		   case 'n':
			daemon = 0;
			break;
		   case 'h':
			usage();
			exit(0);
		   default: /* bad option */
			return 1;
		}
	}

	/* Check that we can open UBC_FILE */
	ub = fopen(UBC_FILE, "r");
	if (ub == NULL)
	{
		fprintf(stderr, "Can't open %s: %s\n",
			UBC_FILE, strerror(errno));
		return 1;
	}
	fclose(ub);

	/* Open and lock log_file */
	of = open(log_file, O_RDWR | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (of == -1)
	{
		fprintf(stderr, "Can't open log file %s: %s\n",
				log_file, strerror(errno));
		return 1;
	}
	if (flock(of, LOCK_EX | LOCK_NB) != 0)
	{
		fprintf(stderr, "Can't lock log file %s: %s\n",
				log_file, strerror(errno));
		return 1;
	}
	if (prep_file(of, reserved_space) != 0)
		return 1;
	if (daemon)
		daemonize();
	/* Set up signal handlers */
	sigusr1.sa_handler = sigusr1h;
	sigusr1.sa_flags = 0;
	sigemptyset(&sigusr1.sa_mask);

	sighup.sa_handler = sighuph;
	sighup.sa_flags = 0;
	sigemptyset(&sighup.sa_mask);

	sigaction(SIGUSR1, &sigusr1, NULL);
	sigaction(SIGHUP, &sighup, NULL);

	/* Start logging */
	start_log(of);
	return 0;
}
