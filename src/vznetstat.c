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
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <vzctl/libvzctl.h>

#include "tc.h"
#define MAX_CLASSES	16

static int round = 0;

enum env_type {
	UNSPEC_TYPE = 0x0,
	VM_TYPE = 0x1,
	CT_TYPE = 0x2,
};

struct uuidmap_t {
	ctid_t ctid;
	int veid;
};

static int ipv6 = 1;
static int ipv4 = 1;
static int _uuidmap_size;
static int pnetstat_mode = 0;
static struct uuidmap_t *_uuidmap;

void usage(void)
{
	if (pnetstat_mode)
		printf("Usage: pnetstat [-4|-6] [-v id] [-c netclassid] [-a] [-r] [-t <ct|vm|all>]\n");
	else
		printf("Usage: vznetstat [-4|-6] [-v id] [-c netclassid] [-a] [-r]\n");
	printf("\t-4\tDisplay ipv4 statistics.\n");
	printf("\t-6\tDisplay ipv6 statistics.\n");
	printf("\t-v\tDisplay stats only for server with given ID.\n");
	printf("\t-c\tDisplay stats only for given network class.\n");
	printf("\t-a\tDisplay all classes.\n");
	printf("\t-r\tDisplay rounded number in K, M or G instead of bytes.\n");
	if (pnetstat_mode)
		printf("\t-t\tDisplay stats only for Containers, virtual machines or both.\n");
	printf("\t-h\tPrints short usage information.\n");
}

void print_header(int type)
{
	if ((type & VM_TYPE) != 0)
		printf("UUID                                 Net.Class     Input(bytes) Input(pkts)        Output(bytes) Output(pkts)\n");
	else
		printf("CTID    Net.Class     Input(bytes) Input(pkts)        Output(bytes) Output(pkts)\n");
}

static int parse_int(const char *str, int *val)
{
	char *tail;

	if (!str || !val)
		return 1;
	errno = 0;
	*val = (int)strtol(str, (char **)&tail, 10);
	if (*tail != '\0' || errno == ERANGE)
		return 1;

	return 0;
}

static int get_veid_by_ctid(ctid_t ctid)
{
	int flags = VZCTL_CONF_SKIP_GLOBAL|VZCTL_CONF_SKIP_PARSE;
	struct vzctl_env_handle *h;
	int ret, veid;

	h = vzctl2_env_open(ctid, flags, &ret);
	if (h == NULL)
		return -1;

	veid = vzctl2_env_get_veid(h);

	vzctl2_env_close(h);

	return veid;
}

static int read_uuid_map(ctid_t ctid)
{
	int n;
	vzctl_ids_t *ctids = NULL;

	ctids = vzctl2_alloc_env_ids();
	if (ctids == NULL)
		return ERR_NOMEM;

	if (EMPTY_CTID(ctid)) {
		n = vzctl2_get_env_ids_by_state(ctids, ENV_STATUS_EXISTS);
	} else {
		n = 1;
		SET_CTID(ctids->ids[0], ctid);
	}

	if (n > 0) {
		_uuidmap = calloc(1, sizeof(struct uuidmap_t) * n);
		if (_uuidmap == NULL)
			return ERR_NOMEM;

		_uuidmap_size = n;
		for (n = 0; n < _uuidmap_size; n++) {
			int veid = get_veid_by_ctid(ctids->ids[n]);
			if (veid != -1) {
				SET_CTID(_uuidmap[n].ctid, ctids->ids[n]);
				_uuidmap[n].veid = veid;
			}
		}
	}

	if (ctids != NULL)
		vzctl2_free_env_ids(ctids);

	return n == -1 ? -1 : 0;
}

static struct uuidmap_t *find_uuid_by_id(unsigned int id)
{
	unsigned int i;

	for (i = 0; i < _uuidmap_size; i++)
		if (_uuidmap[i].veid == id)
			return &_uuidmap[i];
	return NULL;
}

static int print_stat(struct vzctl_tc_get_stat *stat, int len, int class_mask, int type)
{
	int i, ret, class;
	unsigned long long in, out;
	char *sfx[3] = {"K", "M", "G"};
	char *in_sfx, *out_sfx;
	char id[64];
	struct uuidmap_t *map;

	map = find_uuid_by_id(stat->veid);
	if (map != NULL)
		snprintf(id, sizeof(id), "%s", map->ctid);
	else
		snprintf(id, sizeof(id), "%d", stat->veid);

	for (class = 0; class < len; class++) {
		if (!((1 << class) & class_mask))
			continue;
		in_sfx = " ";
		out_sfx = " ";
		in = stat->incoming[class];
		out = stat->outgoing[class];
		if (round) {
			for (i = 0; i < 3 ; i++) {
				if ((in >> 10)) {
					in = in >> 10;
					in_sfx = sfx[i];
				}
				if ((out >> 10)) {

					out = out >> 10;
					out_sfx = sfx[i];
				}
			}
		}

		ret = printf("%-36s %-2u %20llu%1s %10u %20llu%1s %10u\n",
			id,
			class,
			in, in_sfx,
			stat->incoming_pkt[class],
			out, out_sfx,
			stat->outgoing_pkt[class]);
		if (ret < 0)
			return ERR_OUT;
	}
	return 0;
}

static struct vzctl_tc_get_stat *alloc_tc_stat(void)
{
	struct vzctl_tc_get_stat *stat;
	int len;

	if ((len = tc_max_class()) < 0)
		return NULL;

	stat = malloc(sizeof(struct vzctl_tc_get_stat));
	if (stat == NULL)
		return NULL;

	stat->length = len;

	stat->incoming = calloc(len, sizeof(__u64));
	stat->outgoing = calloc(len, sizeof(__u64));
	stat->incoming_pkt = calloc(len, sizeof(__u32));
	stat->outgoing_pkt = calloc(len, sizeof(__u32));

	return stat;
}

static void clear_tc_stat(struct vzctl_tc_get_stat *stat)
{
	bzero(stat->incoming, sizeof(__u64) * stat->length);
	bzero(stat->outgoing, sizeof(__u64) * stat->length);
	bzero(stat->incoming_pkt, sizeof(__u32) * stat->length);
	bzero(stat->outgoing_pkt, sizeof(__u32) * stat->length);
}

static void add_tc_stat(struct vzctl_tc_get_stat *out, struct vzctl_tc_get_stat *s)
{
	unsigned int i;
	for (i = 0; i < out->length; i++) {
		out->incoming[i] += s->incoming[i];
		out->outgoing[i] += s->outgoing[i];
		out->incoming_pkt[i] += s->incoming_pkt[i];
		out->outgoing_pkt[i] += s->outgoing_pkt[i];
	}
}

static void free_tc_stat(struct vzctl_tc_get_stat *stat)
{
	free(stat->incoming);
	free(stat->outgoing);
	free(stat->incoming_pkt);
	free(stat->outgoing_pkt);
	free(stat);
}

int get_stats(struct vzctl_tc_get_stat_list *velist, int class_mask, int type)
{
	int i, ret = 0;
	struct vzctl_tc_get_stat *stat;
	struct vzctl_tc_get_stat *stat6;
	struct uuidmap_t *map;
	char id[64];

	if (velist == NULL)
		return 0;
	stat = alloc_tc_stat();
	stat6 = alloc_tc_stat();
	if (stat == NULL || stat6 == NULL)
		return ERR_NOMEM;

	print_header(type);
	for (i = 0; i < velist->length; i++)
	{
		clear_tc_stat(stat);
		clear_tc_stat(stat6);
		stat->veid = velist->list[i];
		stat6->veid = velist->list[i];
		map = find_uuid_by_id(velist->list[i]);
		if (map != NULL)
			snprintf(id, sizeof(id), "%s", map->ctid);
		else
			snprintf(id, sizeof(id), "%u", stat->veid);

		if (ipv4)
			ret = tc_get_stat(id, stat, 0);
		if (ipv6)
			ret = tc_get_stat(id, stat6, 1);

		if (ret < 0)
			break;

		add_tc_stat(stat, stat6);

		if ((ret = print_stat(stat, ret, class_mask, type)))
			break;
	}
	free_tc_stat(stat);
	free_tc_stat(stat6);
	return ret;
}

int build_class_mask(int *class_mask)
{
	int len, ret, i;

	*class_mask = 1;
	if ((len = tc_get_class_num()) < 0)
		return len;
	if (ipv4 && len) {
		struct vz_tc_class_info *info;

		info = calloc(len, sizeof(struct vz_tc_class_info));
		if (info == NULL)
			return ERR_NOMEM;
		ret = tc_get_classes(info, len);
		if (ret < 0)
			return ret;
		for (i = 0; i < len; i++)
			*class_mask |= (1 << info[i].cid);

		free(info);
	}
	if ((len = tc_get_v6_class_num()) < 0)
		return len;
	if (ipv6 && len) {
		struct vz_tc_class_info_v6 *info;

		info = calloc(len, sizeof(struct vz_tc_class_info_v6));
		if (info == NULL)
			return ERR_NOMEM;
		ret = tc_get_v6_classes(info, len);
		if (ret < 0)
			return ret;
		for (i = 0; i < len; i++)
			*class_mask |= (1 << info[i].cid);

		free(info);
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct vzctl_tc_get_stat_list velist = {NULL, 0};
	int ret, classid;
	int class_mask_p = 0;
	int class_mask = 0;
	int all_classes = 0;
	char c;
	int type = VM_TYPE;
	ctid_t ctid = {};

	pnetstat_mode = !strcmp(basename(argv[0]), "pnetstat");

	if (pnetstat_mode)
		type = VM_TYPE | CT_TYPE;

	while ((c = getopt (argc, argv, "v:c:arht:64")) != -1)
	{
		switch (c)
		{
		case 'v':
			if (vzctl2_parse_ctid(optarg, ctid)) {
				printf("Invalid veid: %s\n", argv[2]);
				return ERR_INVAL_OPT;
			}
			break;
		case 'c':
			if (parse_int(optarg, &classid) ||
				classid > MAX_CLASSES)
			{
				printf("Invalid classid: %s\n", argv[2]);
				return ERR_INVAL_OPT;
			}
			class_mask_p |= 1 << classid;
			break;
		case 'a':
			all_classes = 1;
			break;
		case 'r':
			round = 1;
			break;
		case 't':
			if (optarg[0] == 'a')
				 type = VM_TYPE | CT_TYPE;
			else if (optarg[0] == 'v')
				type = VM_TYPE;
			else if (optarg[0] == 'c')
				type = CT_TYPE;
			else {
				fprintf(stderr, "An invalid type %s specified", optarg);
				return ERR_INVAL_OPT;
			}
			break;
		case '6':
			ipv6 = 1;
			ipv4 = 0;
			break;
		case '4':
			ipv6 = 0;
			ipv4 = 1;
			break;
		case 'h':
			usage();
			exit(0);
		default:
			usage();
			exit(1);
		}
	}

	/* Disable logging */
	vzctl2_set_log_enable(0);

	if (tc_get_v6_class_num() == 0)
		ipv6 = 0;
	if (read_uuid_map(ctid))
		return ERR_READ_UUIDMAP;

	if (!EMPTY_CTID(ctid)) {
		velist.list = realloc(velist.list, sizeof(int));
		if (velist.list == NULL)
			return ERR_NOMEM;

		velist.list[0] = get_veid_by_ctid(ctid);
		velist.length = 1;
	} else if ((ret = get_ve_list(&velist)) < 0)
		return ret;

	if ((ret = build_class_mask(&class_mask)))
		return ret;
	if (all_classes)
		class_mask = ~0;
	else if (class_mask_p)
		class_mask &= class_mask_p;
	ret = get_stats(&velist, class_mask, type);

	return ret;
}
