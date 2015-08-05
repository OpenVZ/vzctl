#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <getopt.h>
#include <string.h>
#include <sys/param.h>
#include <ploop/libploop.h>
#include <vzctl/libvzctl.h>

#include "list.h"
#include "util.h"
#include "vzerror.h"

#define SNAPSHOT_MOUNT_ID       "snap"

#define SNAPSHOT_XML    "Snapshots.xml"
#define GET_SNAPSHOT_XML(buf, ve_private) \
	snprintf(buf, sizeof(buf), "%s/" SNAPSHOT_XML, ve_private);

#define GET_DISK_DESCRIPTOR(buf, ve_private) \
	snprintf(buf, sizeof(buf), "%s/root.hdd/" DISKDESCRIPTOR_XML, ve_private);

struct id_entry {
	list_elem_t list;
	int id;
};

static const char *default_field_order = "parent_uuid,current,uuid,date,name";
static int g_last_field;
static struct ploop_disk_images_data *g_di;
static LIST_HEAD(g_field_order_head);
static LIST_HEAD(g_uuid_list_head);
static int error;
static int g_envid;

/* Handle last field logic */
static const char *FMT(const char *fmt)
{
	return g_last_field ? "%s\n" : fmt;
}

static void print_uuid(struct vzctl_snapshot_data *p)
{
	printf(FMT("%-38s "), p->guid);
}

static void print_date(struct vzctl_snapshot_data *p)
{
	printf(FMT("%-19s "), p->date ? p->date : "");
}

static void print_name(struct vzctl_snapshot_data *p)
{
	printf(FMT("%-32s "), p->name ? p->name : "");
}

static void print_desc(struct vzctl_snapshot_data *p)
{
	printf(FMT("%-32s "), p->desc ? p->desc : "");
}

static void print_parent_uuid(struct vzctl_snapshot_data *p)
{
	printf(FMT("%-38s "), p->parent_guid);
}

static void print_current(struct vzctl_snapshot_data *p)
{
	printf(FMT("%1s "), p->current ? "*" : "");
}

static const char *generate_snapshot_component_name(unsigned int envid,
		const char *data, char *buf, int len)
{
	snprintf(buf, len, "%d-%s-%s", envid, SNAPSHOT_MOUNT_ID, data);
	return buf;
}

static void print_device(struct vzctl_snapshot_data *p)
{
	char buf[MAXPATHLEN];
	char dev[MAXPATHLEN] = "";

	ploop_set_component_name(g_di,
			generate_snapshot_component_name(g_envid, p->guid, buf, sizeof(buf)));

	if (ploop_get_dev(g_di, dev, sizeof(dev)))
		goto err;

	printf(FMT("%-16s "), dev);
	return;
err:
	printf(FMT("%-16s "), "");
	fprintf(stderr, "%s\n", ploop_get_last_error());
	error++;
}

static void print_target(struct vzctl_snapshot_data *p)
{
	char buf[MAXPATHLEN];
	char dev[MAXPATHLEN];

	ploop_set_component_name(g_di,
			generate_snapshot_component_name(g_envid, p->guid, buf, sizeof(buf)));

	if (ploop_get_dev(g_di, dev, sizeof(dev)))
		goto err;
	if (ploop_get_mnt_by_dev(dev, buf, sizeof(buf)))
		goto err;

	printf(FMT("%-32s "), buf);
	return;
err:
	printf(FMT("%-32s "), "");
	fprintf(stderr, "%s\n", ploop_get_last_error());
	error++;
}

struct snapshot_field {
	char *name;
	char *hdr;
	char *fmt;
	void (* print_fn)(struct vzctl_snapshot_data *p);
} field_tbl[] =
{
{"uuid", "UUID", "%-38s", print_uuid},
{"parent_uuid", "PARENT_UUID", "%-38s", print_parent_uuid},
{"current", "C", "%1s", print_current},
{"date", "DATE", "%-19s", print_date},
{"name", "NAME", "%-32s", print_name},
{"desc", "DESC", "%-32s", print_desc},
{"device", "DEVICE", "%-16s", print_device},
{"target", "TARGET", "%-32s", print_target},
};

static void print_names(void)
{
	unsigned int i;

	for (i = 0; i < sizeof(field_tbl) / sizeof(*field_tbl); i++)
		printf("%-15s %-15s\n", field_tbl[i].name,
				field_tbl[i].hdr);
}

static int get_field_tbl_id(const char *name)
{
	unsigned int i;

	for (i = 0; i < sizeof(field_tbl) / sizeof(field_tbl[0]); i++)
		if (!strcasecmp(name, field_tbl[i].name))
			return i;
	return -1;
}

static int add_entry(list_head_t *head, int id)
{
	struct id_entry *p;

	p = malloc(sizeof( struct id_entry));
	if (p == NULL) {
		fprintf(stderr, "ENOMEM\n");
		return 1;
	}
	p->id = id;
	list_add_tail(&p->list, head);
	return 0;
}

static int build_field_order_list(const char *fields)
{
	int id;
	size_t len;
	const char *sp, *ep, *p;
	char name[256];

	sp = fields != NULL ? fields : default_field_order;

	ep = sp + strlen(sp);
	do {
		if ((p = strchr(sp, ',')) == NULL)
			p = ep;
		len = p - sp + 1;
		if (len > sizeof(name) - 1) {
			fprintf(stderr, "Field name %s is unknown.\n", sp);
			return VZ_INVALID_PARAMETER_VALUE;
		}
		snprintf(name, len, "%s", sp);
		sp = p + 1;
		id = get_field_tbl_id(name);
		if (id == -1) {
			fprintf(stderr, "Unknown field: %s\n", name);
			return VZ_INVALID_PARAMETER_VALUE;
		}
		if (add_entry(&g_field_order_head, id))
			return VZ_RESOURCE_ERROR;
	} while (sp < ep);

	return 0;
}

static int build_uuid_list(struct vzctl_snapshot_tree *tree, const char *guid)
{
	int done = 0;
	int id, n;

	// List all snapshots
	if (guid == NULL) {
		for (n = 0; n < tree->nsnapshots; n++) {
			if (add_entry(&g_uuid_list_head, n))
				return VZ_RESOURCE_ERROR;
		}
		return 0;
	}
	for (n = 0; n < tree->nsnapshots; n++) {
		id = vzctl2_find_snapshot_by_guid(tree, guid);
		if (id == -1) {
			fprintf(stderr, "Can't find snapshot by uuid %s\n", guid);
			return VZ_INVALID_PARAMETER_VALUE;
		}

		if (add_entry(&g_uuid_list_head, id))
			return VZ_RESOURCE_ERROR;

		guid = tree->snapshots[id]->parent_guid;
		if (guid == NULL || *guid == '\0') {
			done = 1;
			break;
		}
	}
	if (!done) {
		fprintf(stderr, "Inconsistency detected, base image not found\n");
		return VZCTL_E_LIST_SNAPSHOT;
	}
	return 0;
}

static int read_di(const char *ve_private)
{
	char fname[MAXPATHLEN];

	GET_DISK_DESCRIPTOR(fname, ve_private)
	if (ploop_read_disk_descr(&g_di, fname)) {
		fprintf(stderr, "Failed to read %s: %s\n",
				fname, ploop_get_last_error());
		return 1;
	}

	return 0;
}

static void print_hdr(int no_hdr)
{
	struct id_entry *field_entry = NULL;

	if (no_hdr)
		return;

	list_for_each(field_entry, &g_field_order_head, list) {
		printf(field_tbl[field_entry->id].fmt,
				field_tbl[field_entry->id].hdr);
		if ((void *)g_field_order_head.prev != (void*)field_entry)
			printf(" ");
	}
	printf("\n");
}

static void usage_snapshot_list(int err)
{
	fprintf(err ? stderr : stdout,
		"vzctl snapshot-list <ctid|name> [-H] [-o field[,field...]] [--id <uuid>]\n"
		"vzctl snapshot-list <ctid|name> -L | --list\n"
	       );
}

int vzctl_env_snapshot_list(int argc, char **argv, int envid,
		const char *ve_private)
{
	int c, ret;
	int no_hdr = 0;
	struct vzctl_snapshot_tree *tree = NULL;
	struct id_entry *field_entry = NULL;
	struct id_entry * uuid_entry = NULL;
	const char *output = NULL;
	const char *guid =  NULL;
	char fname[MAXPATHLEN];
	struct option list_options[] =
	{
		{"no-header",	no_argument, NULL, 'H'},
		{"output",	required_argument, NULL, 'o'},
		{"help",	no_argument, NULL, 'h'},
		{"uuid",	required_argument, NULL, 'u'},
			{"id",	required_argument, NULL, 'u'},
		{"list",	no_argument, NULL, 'L'},
		{ NULL, 0, NULL, 0 }
	};

	g_envid = envid;
	while (1) {
		int option_index = -1;
		c = getopt_long(argc, argv, "HhLo:u:",
				list_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'H':
			no_hdr = 1;
			break;
		case 'o' :
			output = optarg;
			break;
		case 'u':
			if (vzctl_get_normalized_guid(optarg, fname, sizeof(fname))) {
				fprintf(stderr, "Invalid guid is specified: %s\n",
						optarg);
				return VZ_INVALID_PARAMETER_VALUE;
			}
			guid = strdup(fname);
			break;
		case 'h':
			usage_snapshot_list(0);
			return 0;
		case 'L':
			print_names();
			return 0;
		default:
			usage_snapshot_list(1);
			return VZ_INVALID_PARAMETER_SYNTAX;
		}
	}

	if (optind < argc) {
		fprintf(stderr, "non-option ARGV-elements: ");
		while (optind < argc)
			fprintf(stderr, "%s ", argv[optind++]);
		fprintf(stderr, "\n");
		return VZ_INVALID_PARAMETER_SYNTAX;
	}

	ret = build_field_order_list(output);
	if (ret)
		return ret;

	if (ve_private == NULL) {
		fprintf(stderr, "VE_PRIVATE is not specified\n");
		return VZ_VE_PRIVATE_NOTSET;
	}

	GET_SNAPSHOT_XML(fname, ve_private)
	if (stat_file(fname) != 1) {
		print_hdr(no_hdr);
		return 0;
	}

	tree = vzctl2_open_snapshot_tree(fname, &ret);
	if (tree == NULL) {
		fprintf(stderr, "Failed to read %s\n", fname);
		goto free_tree;
	}

	ret = build_uuid_list(tree, guid);
	if (ret)
		goto free_tree;

	if (read_di(ve_private)) {
		ret = VZCTL_E_PARSE_DD;
		goto free_tree;
	}

	print_hdr(no_hdr);

	list_for_each(uuid_entry, &g_uuid_list_head, list) {
		list_for_each(field_entry, &g_field_order_head, list) {
			g_last_field = ((void *)g_field_order_head.prev == (void*)field_entry);
			field_tbl[field_entry->id].print_fn(tree->snapshots[uuid_entry->id]);
		}
	}

	ploop_free_diskdescriptor(g_di);
free_tree:
	if (tree)
		vzctl2_close_snapshot_tree(tree);

	return ret;
}
