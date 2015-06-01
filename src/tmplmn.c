/*
 *
 *  Copyright (C) SWsoft, 1999-2007. All rights reserved.
 *
 */

/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <ploop/libploop.h>

#include "vzctl.h"
#include "vzerror.h"
#include "tmplmn.h"
#include "util.h"

char *read_dist(struct CParam *param)
{
	FILE *fd;
	char buf[STR_SIZE];
	int len;
	char *p;
	int status;
	char *argv[6];
	int i = 0;

	argv[i++] = VZPKG;
	argv[i++] = "info";
	argv[i++] = "-q";
	if (param->ostmpl != NULL) {
		argv[i++] = param->ostmpl;
		argv[i++] = "distribution";
	} else
		return NULL;
	argv[i++] = NULL;

	if ((fd = vzctl_popen(argv, NULL, 0)) == NULL) {
		char *str = arg2str(argv);
		logger(-1, errno, "failed: %s ", str);
		free(str);
		return NULL;
	}
	*buf = 0;
	len = fread(buf, 1, sizeof(buf), fd);
	status = vzctl_pclose(fd);
	if (WEXITSTATUS(status) || len <= 0)
		return NULL;
	buf[len] = 0;
	if ((p = strrchr(buf, '\n')) != NULL)
		*p = 0;
	return strdup(buf);

}

char *get_dist_type(struct CParam *param)
{
	char *dist, *tmp;

	if (param->dist != NULL)
		return strdup(param->dist);
	if ((tmp = get_ostmpl(param)) == NULL)
		return NULL;
	if ((dist = read_dist(param)) == NULL)
		dist = strdup(tmp);
	/* store calculated dist */
	param->dist = strdup(dist);

	return dist;
}

char *get_ostmpl(struct CParam *param)
{
	if (param == NULL)
		return NULL;
	if (param->ostmpl != NULL)
		return param->ostmpl;
	return NULL;
}
