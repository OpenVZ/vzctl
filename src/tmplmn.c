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
