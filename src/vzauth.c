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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "vzctl/libvzctl.h"
#include "vzctl/vzerror.h"

#define ERR_PARAM       1
#define ERR_AUTH        3
#define ERR_READ_PASSWD 5
#define ERR_PSASHADOW   7
#define ERR_GID_CHECK   9


void usage(void)
{
	fprintf(stderr, "Usage: vzauth [-t <type>] [-g <gid>] ctid user\n");
	fprintf(stderr, "\t -t <type>\tsystem|pleskadmin\n");
	fprintf(stderr, "\t -g <gid>\tcheck group id\n");
	return;
}

char *read_passwd(void)
{
	FILE *fp;
	char str[512];
	char *ch;

	if ((fp = fopen("/dev/stdin", "r")) == NULL)
	{
		fprintf(stderr, "Unable to open /dev/stdin: %s\n",
			strerror(errno));
		return NULL;
	}
	*str = 0;
	if (fgets(str, sizeof(str), fp) == NULL && !feof(fp))
	{
		fprintf(stderr, "Error in read pasword from stdin: %s\n",
			strerror(errno));
		return NULL;
	}
	if ((ch = strrchr(str, '\n')) != NULL)
		*ch = 0;
	fclose(fp);
	return strdup(str);
}

static int parse_int(const char *str, int *val)
{
	char *tail;

	errno = 0;
	*val = (int)strtol(str, &tail, 10);
	if (*tail != '\0' || errno == ERANGE)
		return 1;
	return 0;
}

int main(int argc, char **argv)
{
	char *user, *passwd;
	int ret;
	int type = 0, i = 1;
	int gid = -1;
	struct vzctl_env_handle *h;
	ctid_t ctid;

	while (i < argc - 1) {
		if (!strcmp(argv[i], "-t")) {
			i++;
			if (!strcmp(argv[i], "system"))
				type = 0;
			else if (!strcmp(argv[i], "pleskadmin"))
				type = 1;
			else {
				fprintf(stderr, "unknown type: %s\n", argv[i]);
				exit(ERR_PARAM);
			}
		} else if (!strcmp(argv[i], "-g")) {
			i++;

			if (parse_int(argv[i], &gid)) {
				 fprintf(stderr, "An invalid gid");
				 exit(ERR_PARAM);
			}
		} else
			break;
		i++;
	}
	if (i + 2 != argc) {
		usage();
		exit(ERR_PARAM);
	}

	if (vzctl2_parse_ctid(argv[i], ctid)) {
		fprintf(stderr, "Invalid VEID %s\n", argv[i]);
		return ERR_PARAM;
	}
	user = argv[++i];

	if ((passwd = read_passwd()) == NULL)
		return ERR_READ_PASSWD;

	vzctl2_init_log("vzauth");
	vzctl2_set_flags(VZCTL_FLAG_DONT_USE_WRAP);

	if (vzctl2_lib_init())
		return ERR_PARAM;

	h = vzctl2_env_open(ctid, 0, &ret);
	if (ret)
		return ERR_PARAM;

	ret = vzctl2_env_auth(h, user, passwd, gid, type);
	if (ret) {
		switch (ret) {
		case VZCTL_E_AUTH_GUID:
			ret = ERR_GID_CHECK;
			break;
		case VZCTL_E_AUTH_PSASHADOW:
			ret = ERR_PSASHADOW;
			break;
		default:	
			ret = ERR_AUTH;
			break;
		}
	}

	vzctl2_env_close(h);

	fprintf(stderr, "%s\n", !ret ? "Access granted." : "Access denied.");

	return ret;
}
