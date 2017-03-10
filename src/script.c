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
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "config.h"

char *makecmdline(char **argv)
{
	char **p;
	char *str, *sp;
	int len = 0;

	if (!argv)
		return NULL;
	p = argv;
	while (*p)
		len += strlen(*p++) + 1;
	if ((str = (char *)xmalloc(len + 1)) == NULL)
		return NULL;
	p = argv;
	sp = str;
	while (*p)
		sp += sprintf(sp, "%s ", *p++);
	return str;
}

void freearg(char **arg)
{
	char **p = arg;
	if (arg == NULL)
		return;
	while (*p)
		free(*p++);
	free(arg);
	return;
}

char *readscript(char *name, int use_f)
{
	struct stat st;
	char *buf = NULL, *p = NULL;
	int fd, len;

	if (!name)
		return NULL;
	/* Read include file first */
	if (use_f)
		buf = readscript(VE_FUNC, 0);

	if (stat(name, &st))
		return NULL;

	if (!(fd = open(name, O_RDONLY)))
	{
		logger(-1, errno, "Error opening %s", name);
		return NULL;
	}
	if (buf)
	{
		len = st.st_size + strlen(buf);
		buf = (char *)realloc(buf, len + 2);
		p = buf + strlen(buf);
	}
	else
	{
		len = st.st_size;
		buf = (char *)realloc(buf, len + 2);
		p = buf;
	}

	buf[len] = '\n';
	buf[len + 1] = 0;
	if (read(fd, p, st.st_size) < 0)
	{
		logger(-1, errno, "Error reading %s", name);
		free(buf);
		return NULL;
	}
	close(fd);
	return buf;
}

