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

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/utsname.h>

#include "vzctl.h"

struct env_feature {
	char *name;
	unsigned long long mask;
};
static struct env_feature env_features[] = {
	{ "nfs",	VZ_FEATURE_NFS},
	{ "sit",	VZ_FEATURE_SIT},
	{ "ipip",	VZ_FEATURE_IPIP},
	{ "ppp",	VZ_FEATURE_PPP},
	{ "ipgre",	VZ_FEATURE_IPGRE},
	{ "bridge",	VZ_FEATURE_BRIDGE},
	{ "nfsd",	VZ_FEATURE_NFSD},
	{ "time",	VZ_FEATURE_TIME},

	{ NULL}
};

void features_mask2str(unsigned long long mask, unsigned long long known, char *delim,
		char *buf, int len)
{
	struct env_feature *feat;
	int ret, i, j = 0;

	for (i = 0, feat = env_features; feat->name != NULL; feat++) {
		if (!(known & feat->mask))
			continue;
		ret = snprintf(buf, len, "%s%s:%s",
				j++ == 0 ? "" : delim,
				feat->name,
				mask & feat->mask ? "on" : "off");
		buf += ret;
		len -= ret;
		if (len <= 0)
			break;
		i++;
	}
}

void print_json_features(unsigned long long mask, unsigned long long known)
{
	struct env_feature *feat;
	int i, j = 0;

	for (i = 0, feat = env_features; feat->name != NULL; feat++) {
		if (!(known & feat->mask))
			continue;
		printf("%s      \"%s\": %s",
			j++ == 0 ? "{\n" : ",\n",
			feat->name,
			mask & feat->mask ? "true" : "false");
		i++;
	}
	if (j)
		printf("\n    }");
	else
		printf("null");
}
