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
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vzctl/libvzctl.h>
#include <nftables/libnftables.h>

#include "tc.h"
#include "util.h"

int get_ipaddr(char *name, unsigned int *ip)
{
	struct in_addr addr;

	if (name == NULL)
		return ERR_BAD_IP;
	if (!inet_aton(name, &addr))
		return ERR_BAD_IP;
	*ip = addr.s_addr;
	return 0;
}

static int get_prefix(unsigned int mask)
{
	int i, bits = 0;

	for (i = 0; i < 32; i++) {
		if (!mask)
			break;

		bits++;
		mask >>= 1;
	}

	return bits;
}

static int get_prefix_v6(unsigned int *mask)
{
	int i, bits = 0;

	for (i = 0; i < 4; i++)
		bits += get_prefix(mask[i]);

	return bits;
}

static void prefix_to_mask(int prefix, unsigned int *mask)
{
	int i = 0;
	unsigned char *p = (unsigned char *)mask;

        while (prefix >= 8) {    	    
            p[i++] = 0xff;
            prefix -= 8;
        }

        if (prefix)
            p[i++] = ~(unsigned char)0 << (8 - prefix);

        while (i < 16)
            p[i++] = 0;
}

static int run_nft_cmd(const char *cmd, char *out, int len)
{
	int ret = -1;
	struct nft_ctx *nft;

	if (!(nft = nft_ctx_new(NFT_CTX_DEFAULT)))
		return -1;

	if (nft_ctx_buffer_output(nft) || nft_ctx_buffer_error(nft))
		goto out;

	if (nft_run_cmd_from_buffer(nft, cmd))
		goto err;

	if (out && len) {
		const char *p = nft_ctx_get_output_buffer(nft);
		strncpy(out, p, len);
		out[len - 1] = '\0';
	}

	ret = 0;

err:
	nft_ctx_unbuffer_output(nft);
	nft_ctx_unbuffer_error(nft);
out:
	nft_ctx_free(nft);

	return ret;
}

static int tc_check_ve_classes(void)
{
	if (run_nft_cmd("list table netdev ve_classes", NULL, 0)) {
		if (run_nft_cmd("create table netdev ve_classes", NULL, 0)) {
		
			fprintf(stderr, "Unable to create table of ve classes: %s\n",
				strerror(errno));
			return ERR_IOCTL;
		}
	}

	return 0;
}

static int tc_clear_ve_classes(void)
{
	if (!run_nft_cmd("list table netdev ve_classes", NULL, 0)) {
		if (run_nft_cmd("delete table netdev ve_classes", NULL, 0)) {
		
			fprintf(stderr, "Unable to clear table of ve classes: %s\n",
				strerror(errno));
			return ERR_IOCTL;
		}
	}

	return 0;
}

int tc_set_classes(struct vz_tc_class_info *info, int length)
{
	int i, ret;

	if ((ret = tc_clear_ve_classes()))
		return ret;

	if ((ret = tc_check_ve_classes()))
		return ret;

	for (i = 0; i < length; i++) {
		char cmd[256];
		char ip[32];

		snprintf(cmd, sizeof(cmd), "list set netdev ve_classes net_class4_%u",
			 info[i].cid);
		if (run_nft_cmd(cmd, NULL, 0)) {
			snprintf(cmd, sizeof(cmd), "add set netdev ve_classes net_class4_%u"
				 " { type ipv4_addr ; flags interval ; }", info[i].cid);
			if (run_nft_cmd(cmd, NULL, 0)) {
				fprintf(stderr, "Unable to create class: %s\n",
					strerror(errno));
				return ERR_IOCTL;
			}
		}

		if (inet_ntop(AF_INET, &info[i].addr, ip, sizeof(ip)) == NULL) {
			fprintf(stderr, "Unable to convert element of class: %s\n",
				strerror(errno));
			return ERR_IOCTL;
		}

		snprintf(cmd, sizeof(cmd), "add element netdev ve_classes net_class4_%u"
			 " { %s/%d }", info[i].cid, ip, get_prefix(info[i].mask));
		if (run_nft_cmd(cmd, NULL, 0)) {
			fprintf(stderr, "Unable to create element of class: %s\n",
				strerror(errno));
			return ERR_IOCTL;
		}
	}

	return 0;
}

int tc_set_v6_classes(struct vz_tc_class_info_v6 *info, int length)
{
	int i, ret;

	if ((ret = tc_check_ve_classes()))
		return ret;

	for (i = 0; i < length; i++) {
		char cmd[256];
		char ip[64];

		snprintf(cmd, sizeof(cmd), "list set netdev ve_classes net_class6_%u",
			 info[i].cid);
		if (run_nft_cmd(cmd, NULL, 0)) {
			snprintf(cmd, sizeof(cmd), "add set netdev ve_classes net_class6_%u"
				 " { type ipv6_addr ; flags interval ; }", info[i].cid);
			if (run_nft_cmd(cmd, NULL, 0)) {
				fprintf(stderr, "Unable to create class: %s\n",
					strerror(errno));
				return ERR_IOCTL;
			}
		}

		if (inet_ntop(AF_INET6, &info[i].addr, ip, sizeof(ip)) == NULL) {
			fprintf(stderr, "Unable to convert element of class: %s\n",
				strerror(errno));
			return ERR_IOCTL;
		}

		snprintf(cmd, sizeof(cmd), "add element netdev ve_classes net_class6_%u"
			 " { %s/%d }", info[i].cid, ip, get_prefix_v6(info[i].mask));
		if (run_nft_cmd(cmd, NULL, 0)) {
			fprintf(stderr, "Unable to create element of class: %s\n",
				strerror(errno));
			return ERR_IOCTL;
		}
	}

	return 0;
}

static int tc_get_elements(int class, char *p1, char *p2,
			   struct vz_tc_class_info *info, int *cnt, int length)
{
	char ipaddr[32];
	int prefix;
	const char *full = "0.0.0.0-255.255.255.255";

	if (!strncmp(p1, full, strlen(full))) {
		if (info && *cnt < length) {
			info[*cnt].cid = class;
			info[*cnt].addr = 0;
			info[*cnt].mask = 0;
		}

		(*cnt)++;

		return 0;
	}

	while (sscanf(p1, "%31[.0123456789]/%u[, ]", ipaddr, &prefix) == 2) {
		char *p;

		if (info && *cnt < length) {
			unsigned int mask = 0;
			unsigned int ip[4];

			if (get_netaddr(ipaddr, ip) != AF_INET)
				return -1;

			if (prefix)
				mask = htonl(~(__u32)0 << (32 - prefix));

			info[*cnt].cid = class;
			info[*cnt].addr = ip[0] & mask;
			info[*cnt].mask = mask;
		}

		(*cnt)++;

		if ((p = strstr(p1, ",")) == NULL || p > p2)
			break;

		if ((p1 = strpbrk(p, "0123456789")) == NULL || p1 > p2)
			break;
	}

	return 0;
}

int tc_get_classes(struct vz_tc_class_info *info, int length)
{
	int ret;
	int cnt = 0;
	char out[4096];
	char *p;
	const char *prefix = "set net_class4_";
	const char *elems = "elements = { ";
	const char *endl = " }";

	if ((ret = tc_check_ve_classes()))
		return ret;

	if (run_nft_cmd("list table netdev ve_classes", out, sizeof(out))) {
		fprintf(stderr, "Unable to get classes: %s\n", strerror(errno));
		return ERR_IOCTL;
	}

	p = out;
	while (p) {
		if ((p = strstr(p, prefix)) != NULL) {
			int class;

			p += strlen(prefix);
			if (sscanf(p, "%u_", &class) == 1) {
				char *p1, *p2;

				if ((p1 = strstr(p, elems)) != NULL &&
				    (p2 = strstr(p, endl)) != NULL &&
				    p1 < p2) {
					p1 += strlen(elems);
					if (tc_get_elements(class, p1, p2, info, &cnt, length))
						return ERR_IOCTL;
				}
			}
		}
	}

	return cnt;
}

static int tc_get_v6_elements(int class, char *p1, char *p2,
			      struct vz_tc_class_info_v6 *info, int *cnt, int length)
{
	char ipaddr[64];
	int prefix;
	const char *full = "::-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff";

	if (!strncmp(p1, full, strlen(full))) {
		if (info && *cnt < length) {
			int i;

			info[*cnt].cid = class;

			for (i = 0; i < 4; i++) {
	    			info[*cnt].addr[i] = 0;
				info[*cnt].mask[i] = 0;
			}
		}

		(*cnt)++;

		return 0;
	}

	while (sscanf(p1, "%63[:0123456789abcdef]/%u[, ]", ipaddr, &prefix) == 2) {
		char *p;

		if (info && *cnt < length) {
			unsigned int ip[4];
			int i;

			if (get_netaddr(ipaddr, ip) != AF_INET6)
				return -1;

			info[*cnt].cid = class;

			prefix_to_mask(prefix, info[*cnt].mask);
			for (i = 0; i < 4; i++)
				info[*cnt].addr[i] = ip[i] & info[*cnt].mask[i];
		}

		(*cnt)++;

		if ((p = strstr(p1, ",")) == NULL || p > p2)
			break;

		if ((p1 = strpbrk(p, "0123456789abcdef")) == NULL || p1 > p2)
			break;
	}

	return 0;
}

int tc_get_v6_classes(struct vz_tc_class_info_v6 *info, int length)
{
	int ret;
	int cnt = 0;
	char out[4096];
	char *p;
	const char *prefix = "set net_class6_";
	const char *elems = "elements = { ";
	const char *endl = " }";

	if ((ret = tc_check_ve_classes()))
		return ret;

	if (run_nft_cmd("list table netdev ve_classes", out, sizeof(out))) {
		fprintf(stderr, "Unable to get classes: %s\n", strerror(errno));
		return ERR_IOCTL;
	}

	p = out;
	while (p) {
		if ((p = strstr(p, prefix)) != NULL) {
			int class;

			p += strlen(prefix);
			if (sscanf(p, "%u_", &class) == 1) {
				char *p1, *p2;

				if ((p1 = strstr(p, elems)) != NULL &&
				    (p2 = strstr(p, endl)) != NULL &&
				    p1 < p2) {
					p1 += strlen(elems);
					if (tc_get_v6_elements(class, p1, p2,
							       info, &cnt, length))
						return ERR_IOCTL;
				}
			}
		}
	}

	return cnt;
}

int tc_get_stat(ctid_t ctid, struct vzctl_tc_get_stat *stat, int v6)
{
	int ret;
	struct vzctl_env_handle *h;
	struct vzctl_tc_netstat tc_stat;

	if (!(h = vzctl2_env_open(ctid, VZCTL_CONF_SKIP_PARSE, &ret))) {
		fprintf(stderr, "Unable to get Container %s statistics: %s\n",
			ctid, strerror(errno));
		return ERR_IOCTL;
	}

	if (vzctl2_get_env_tc_netstat(h, &tc_stat, v6) < 0) {
		vzctl2_env_close(h);
		fprintf(stderr, "Unable to get Container %s statistics: %s\n",
			ctid, strerror(errno));
		return ERR_IOCTL;
	}

	vzctl2_env_close(h);

	for (ret = 0; ret < stat->length && ret < TC_MAX_CLASSES; ret++) {
		stat->incoming[ret] = tc_stat.incoming[ret];
		stat->outgoing[ret] = tc_stat.outgoing[ret];
		stat->incoming_pkt[ret] = tc_stat.incoming_pkt[ret];
		stat->outgoing_pkt[ret] = tc_stat.outgoing_pkt[ret];
	}

	return ret;
}

int tc_destroy_ve_stats(ctid_t ctid)
{
	int ret;
	struct vzctl_env_handle *h;

	if (!(h = vzctl2_env_open(ctid, VZCTL_CONF_SKIP_PARSE, &ret))) {
		fprintf(stderr, "Reset failed: %s\n", strerror(errno));
		return ERR_IOCTL;
	}

	if (vzctl2_clear_ve_netstat(h) < 0) {
		vzctl2_env_close(h);
		fprintf(stderr, "Reset failed: %s\n", strerror(errno));
		return ERR_IOCTL;
	}

	printf("Counters for Container %s reseted.\n", ctid);
	vzctl2_env_close(h);

	return 0;
}

int tc_destroy_all_stats(void)
{
	if (vzctl2_clear_all_ve_netstat() < 0) {
		fprintf(stderr, "Reset failed: %s\n", strerror(errno));
		return ERR_IOCTL;
	}

	printf("Counters reseted.\n");
	return 0;
}

int tc_get_class_num(void)
{
	return tc_get_classes(NULL, 0);
}

int tc_get_v6_class_num(void)
{
	return tc_get_v6_classes(NULL, 0);
}

int tc_get_base(int veid)
{
	return 0;
}

int tc_max_class(void)
{
	return TC_MAX_CLASSES;
}

int get_ve_list(struct vzctl_tc_get_stat_list *velist)
{
	int len = 0;
	int max = 512;
	char out[4096];
	char *p;
	const char *prefix = "netdev ve_";

	if (run_nft_cmd("list tables netdev", out, sizeof(out))) {
		fprintf(stderr, "Unable to get Container list: %s\n",
			strerror(errno));
		return ERR_IOCTL;
	}

	velist->list = calloc(len, sizeof(int));

	p = out;
	while (p) {
		if ((p = strstr(p, prefix)) != NULL) {
			if (len == max - 1) {
				max += 512;
				velist->list = realloc(velist->list,
						       max * sizeof(envid_t));
			}

			if (sscanf(p + strlen(prefix),
				   (strspn(p + strlen(prefix), "0123456789abcdef") == 32) ?
				   "%08x" : "%u",
				   &velist->list[len]) == 1) {
				velist->list[len] &= 0x7fffffff;
				len++;
			}
		}

		if (p)
			p++;
	}

	velist->length = len;

	return len;
}
