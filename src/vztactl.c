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
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <linux/vzctl_venet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <vzctl/libvzctl.h>

#include "tc.h"
#include "util.h"
#include "clist.h"

static int sort;
static int ipv6;

void usage(void)
{
	printf("Usage: vztactl <command> [<options>]\n"
"\tclass_load [-s] <net_classes_file>\n"
"\tclass_unload\n"
"\tclass_list\n"
"\treset_ve_stat <ctid>\n"
"\treset_all_stat\n");
}

void print_classes(struct vz_tc_class_info *info, int len)
{
	int i;
	char ip[42];
	char mask[42];

	for (i = 0; i < len; i++)
	{
		if (inet_ntop(AF_INET, &info[i].addr, ip, sizeof(ip)) == NULL)
			continue;
		if (inet_ntop(AF_INET, &info[i].mask, mask, sizeof(mask)) == NULL)
			*mask = 0;
		printf("%-5d %-40s %-s\n",
			info[i].cid, ip, mask);
	}
}

void print_v6_classes(struct vz_tc_class_info_v6 *info, int len)
{
	int i;
	char ip[42];
	char mask[42];

	for (i = 0; i < len; i++)
	{
		if (inet_ntop(AF_INET6, info[i].addr, ip, sizeof(ip)) == NULL)
			continue;
		if (inet_ntop(AF_INET6, info[i].mask, mask, sizeof(mask)) == NULL)
			*mask = 0;
		printf("%-5d %-40s %-8.8x%-8.8x%-8.8x%-8.8x\n",
				info[i].cid, ip, info[i].mask[0], info[i].mask[1], info[i].mask[2], info[i].mask[3]);
	}
}

int print_ve_stats(struct vzctl_tc_get_stat *stat)
{
	int i, ret;

	for (i = 0; i < stat->length; i++)
	{
		ret = printf("%-10d %-5u %20llu %20llu\n",
			stat->veid, i,
			stat->incoming[i],
			stat->outgoing[i]);
		if (ret < 0)
			return ERR_OUT;
	}
	return 0;
}

int net_sort_fn(const void *val1, const void *val2)
{
	const struct vz_tc_class_info *i1 = val1;
	const struct vz_tc_class_info *i2 = val2;
	int res;

	res = memcmp(&i2->mask, &i1->mask, sizeof(i1->mask));
	if (res == 0)
		return 0;
	// Move mask 0 to the end
	if (i1->mask == 0)
		res = 1;

	/* sort by ascending mask*/
	return (res * -1);
}


int net_v6_sort_fn(const void *val1, const void *val2)
{
	const struct vz_tc_class_info_v6 *i1 = val1;
	const struct vz_tc_class_info_v6 *i2 = val2;
	int res;

	res = memcmp(i2->mask, i1->mask, sizeof(i1->mask));
	if (res == 0)
		return 0;

	// Move mask 0 to the end
	if (i1->mask[0] == 0 && i1->mask[1] == 0 && i1->mask[2] == 0 && i1->mask[3] == 0)
		res = 1;

	/* sort by ascending mask*/
	return (res * -1);
}

int read_network_classes(char *path, struct CList **list)
{
	FILE *fp;
	char buf[256];
	unsigned int class;
	unsigned int mask, mask_max;
	unsigned long *_mask, *_class;
	unsigned int ip[4];
	char ipaddr[42];
	int max_class;
	int family;
	struct CList *classes = NULL;

	if ((fp = fopen(path, "r")) == NULL)
	{
		printf("Unable open %s: %s\n", path, strerror(errno));
		return ERR_TC_READ_CLASSES;
	}
	if ((max_class = tc_max_class()) < 0)
		return max_class;
	while (fgets(buf, sizeof(buf), fp))
	{
		if (buf[0] == '#')
			continue;
		if (sscanf(buf, "%u%*[\t ]%41[^/]/%u",
				&class, ipaddr, &mask) != 3)
		{
			continue;
		}
		if (class > max_class)
		{
			printf("Error: invalid class number %d, should be <= %d, skipped..."
					"\n",	class, max_class);
			continue;
		}
		if ((family = get_netaddr(ipaddr, ip)) == -1)
		{
			printf("Error: Invalid ip %s skipped...\n", ipaddr);
			continue;
		}
		if (family == AF_INET)
			mask_max = 32;
		else
			mask_max = 128;
		mask = (mask > mask_max) ? mask_max : mask;

		_mask = malloc(sizeof(unsigned long));
		*_mask = mask;
		_class = malloc(sizeof(unsigned long));
		*_class = class;
		classes = ListAddElem(classes, ipaddr, _class, _mask);
	}

	fclose(fp);
	*list = classes;
	return 0;
}

void v6mask2str(int mask, char *buf)
{
	int i, j;

	if (mask > 128)
		mask = 128;

	for (i = 1, j = 0; i <= 16; i++) {
		if (mask <= (i * 8)) {
			char n;
			n = mask - (i - 1) * 8;
			n = ~(~0 << n);
			snprintf(&buf[j], 3, "%2.2x", n);
			j += 2;
			if (i != 16) {
				sprintf(&buf[j], "::");
				j += 2;
			}
			break;
		}
		buf[j++] = 'f';
		buf[j++] = 'f';
		if (!(i % 2))
			buf[j++] = ':';
	}
	buf[j] = 0;
}

static int set_classes_from_list(struct CList *classes)
{
	struct vz_tc_class_info *info = NULL;
	struct CList *l;
	int family, i, ret;
	int len = 0;

	for (i = 0; i < 2; i++) {
		for (l = classes; l != NULL; l = l->next) {
			unsigned int class = *l->val1;
			unsigned int mask = *l->val2;
			unsigned int ip[4];

			// Add clas 0 to the end
			if ((i == 0 && class == 0) ||
			    (i == 1 && class != 0))
				continue;

			family = get_netaddr(l->str, ip);
			if (family != AF_INET)
				continue;

			if (mask != 0)
				mask = htonl(~(__u32)0 << (32 - mask));
			info = realloc(info, sizeof(struct vz_tc_class_info) * (len + 1));
			if (info == NULL) {
				fprintf(stderr, "Out of memory\n");
				return -1;
			}
			info[len].cid = class;
			info[len].addr = ip[0] & mask;
			info[len].mask = mask;
			len++;
		}
		if (i == 0 && sort && len > 0)
			qsort(info, len, sizeof(struct vz_tc_class_info), net_sort_fn);
	}

	print_classes(info, len);
	ret = tc_set_classes(info, len);

	return ret;
}

static void v6maskip(__u32 *ip, __u32 *mask)
{
	ip[0] &= mask[0];
	ip[1] &= mask[1];
	ip[2] &= mask[2];
	ip[3] &= mask[3];
}

static int set_v6_classes_from_list(struct CList *classes)
{
	struct vz_tc_class_info_v6 *info = NULL;
	struct CList *l;
	int family, i, ret;
	int len = 0;

	for (i = 0; i < 2; i++) {
		for (l = classes; l != NULL; l = l->next) {
			unsigned int class = *l->val1;
			unsigned int ip[4];
			char buf[128];

			// Add clas 0 to the end
			if ((i == 0 && class == 0) ||
			    (i == 1 && class != 0))
				continue;

			family = get_netaddr(l->str, ip);
			if (family != AF_INET6)
				continue;

			info = realloc(info, sizeof(struct vz_tc_class_info_v6) * (len + 1));
			if (info == NULL) {
				fprintf(stderr, "Out of memory\n");
				return -1;
			}
			info[len].cid = class;
			memcpy(info[len].addr, ip, sizeof(info->addr));
			v6mask2str(*l->val2, buf);
			if (inet_pton(AF_INET6, buf, info[len].mask) < 0) {
				fprintf(stderr, "failed to convert: %s\n", buf);
				return -1;
			}
			v6maskip(info[len].addr, info[len].mask);
			len++;
		}
		if (i == 0 && sort && len > 0)
			qsort(info, len, sizeof(struct vz_tc_class_info_v6), net_v6_sort_fn);
	}
	print_v6_classes(info, len);
	ret = tc_set_v6_classes(info, len);

	return ret;
}

int read_global(void)
{
	int ret;
	const char *p;
	struct vzctl_env_handle *h;
	char conf[1024];

	vzctl2_get_global_conf_path(conf, sizeof(conf));
	h = vzctl2_env_open_conf(0, conf, VZCTL_CONF_SKIP_GLOBAL|VZCTL_CONF_BASE_SET, &ret);
	if (h == NULL)
		return -1;

	p = NULL;
	vzctl2_env_get_param(h, "IPV6", &p);
	if (p != NULL && !strcmp(p, "yes"))
		ipv6 = 1;

	vzctl2_env_close(h);

	return ret;
}

int classes_load(char *path)
{
	struct CList *classes = NULL;
	int ret;

	read_global();

	if ((ret = read_network_classes(path, &classes)) < 0)
		return ret;
	if ((ret = set_classes_from_list(classes)))
		return ret;
	if (ipv6) {
		if ((ret = set_v6_classes_from_list(classes)))
			return ret;
	}
	printf("Classes loaded successfully.\n");

	return 0;
}

int classes_unload(void)
{
	struct vz_tc_class_info info;
	struct vz_tc_class_info_v6 info_v6;
	int ret;

	memset(&info, 0, sizeof(info));
	if ((ret = tc_set_classes(&info, 0)))
		return ret;
	memset(&info_v6, 0, sizeof(info_v6));
	if ((ret = tc_set_v6_classes(&info_v6, 0)))
		return ret;

	printf("Classes unloaded successfully.\n");
	return 0;
}

int get_classes(void)
{
	int len, ret;

	if ((len = tc_get_class_num()) < 0)
		return len;

	printf("class addr             mask\n");
	if (len != 0) {
		struct vz_tc_class_info *info;

		info = calloc(len, sizeof(struct vz_tc_class_info));
		if (info == NULL)
			return ERR_NOMEM;
		if ((ret = tc_get_classes(info, len)) < 0)
			return ret;
		print_classes(info, ret);
		free(info);
	}

	if ((len = tc_get_v6_class_num()) < 0)
		return len;
	if (len != 0) {
		struct vz_tc_class_info_v6 *info_v6;

		info_v6 = calloc(len, sizeof(struct vz_tc_class_info_v6));
		if (info_v6 == NULL)
			return ERR_NOMEM;
		if ((ret = tc_get_v6_classes(info_v6, len)) < 0)
			return ret;
		print_v6_classes(info_v6, len);
		free(info_v6);
	}
	return 0;
}

int parse_opt(int argc, char **argv)
{
	int ret = 0;
	ctid_t ctid;

	if (argc < 2)
	{
		usage();
		return ERR_TC_INVAL_CMD;
	}
	if (!strcmp(argv[1], "class_load"))
	{
		char *file = NULL;

		if (argc == 3) {
			file = argv[2];
		}
		else if (argc == 4) {
			if (strcmp(argv[2], "-s")) {
				fprintf(stderr, "Unknown option %s\n", argv[2]);
				return ERR_INVAL_OPT;
			}
			file = argv[3];
			sort = 1;
		}
		else {
			usage();
			return ERR_INVAL_OPT;
		}
		ret = classes_load(file);
	}
	else if (!strcmp(argv[1], "class_unload"))
	{
		ret = classes_unload();
	}
	else if (!strcmp(argv[1], "class_list"))
	{
		ret = get_classes();
	}
	else if (!strcmp(argv[1], "get_ve_stat"))
	{
		printf("vznetstat utility should be used to get CT statistic\n");
		return ERR_INVAL_OPT;
	}
	else if (!strcmp(argv[1], "reset_ve_stat"))
	{
		if (argc != 3)
		{
			usage();
			return ERR_INVAL_OPT;
		}
		if (vzctl2_parse_ctid(argv[2], ctid))
		{
			printf("Invalid veid: %s\n", argv[2]);
			return ERR_INVAL_OPT;
		}
		ret = tc_destroy_ve_stats(ctid);
	}
	else if (!strcmp(argv[1], "reset_all_stat"))
	{
			ret = tc_destroy_all_stats();
	}
	else if (!strcmp(argv[1], "event"))
	{
		if (argc != 3)
			return ERR_INVAL_OPT;
		if (!strcmp(argv[2], "shaperrestart"))
			vzctl2_send_state_evt(0, VZCTL_NET_SHAPING_CONFIG_CHANGED);
		else if (!strcmp(argv[2], "accrestart"))
			vzctl2_send_state_evt(0, VZCTL_NET_CLASSES_CONFIG_CHANGED);
		else
			printf("Unknown event: %s\n", argv[2]);
	}
	else
	{
		usage();
		ret = ERR_TC_INVAL_CMD;
	}
	return ret;
}

int main(int argc, char **argv)
{
	int ret;

	if (argc == 1)
	{
		usage();
		return 10;
	}
	ret = parse_opt(argc, argv);
	return (ret < 0) ? ret * -1 : ret;
}
