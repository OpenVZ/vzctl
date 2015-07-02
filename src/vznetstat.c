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
#include <linux/vzctl_venet.h>
#include <linux/vzctl_netstat.h>
#include <errno.h>
#include <vzctl/libvzctl.h>

#include "tc.h"
#define MAX_CLASSES	16

int __vzctlfd;
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

static int read_uuid_map(void)
{
	int ret, n;
	struct vzctl_env_handle *h;
	int flags = VZCTL_CONF_SKIP_GLOBAL | VZCTL_CONF_BASE_SET;
	vzctl_ids_t *ctids = vzctl2_alloc_env_ids();

	if (ctids == NULL)
		return -1;
	
	n = vzctl2_get_env_ids_by_state(ctids, ENV_STATUS_EXISTS);
	if (n > 0) {
		_uuidmap = malloc(sizeof(struct uuidmap_t) * n);
		if (_uuidmap == NULL)
			return -1;

		_uuidmap_size = n;
		for (n = 0; n < _uuidmap_size; n++) {
			h = vzctl2_env_open(ctids->ids[n], flags, &ret);
			if (ret)
				continue;

			SET_CTID(_uuidmap[n].ctid, ctids->ids[n]);
			_uuidmap[n].veid = vzctl2_env_get_veid(h);

			vzctl2_env_close(h);
		}
	}

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

static struct uuidmap_t *find_id_by_uuid(const char *ctid)
{
	unsigned int i;

	for (i = 0; i < _uuidmap_size; i++)
		if (!strcmp(_uuidmap[i].ctid, ctid))
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

		if (ipv4)
			ret = tc_get_stat(stat);
		if (ipv6)
			ret = tc_get_v6_stat(stat6);

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
	if ((__vzctlfd = open(VZCTLDEV, O_RDWR)) < 0)
	{
		printf("Unable to open %s: %s\n", VZCTLDEV, strerror(errno));
		return 7;
	}
	if (tc_get_v6_class_num() == 0)
		ipv6 = 0;
	if (read_uuid_map())
		return ERR_READ_UUIDMAP;

	if (!EMPTY_CTID(ctid)) {
		struct uuidmap_t *map = find_id_by_uuid(ctid);

		velist.list = realloc(velist.list, sizeof(int));
		velist.list[0] = map != NULL ? map->veid : -1;
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
