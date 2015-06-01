#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#include "vzctl.h"
#include "util.h"
#include "vzerror.h"
#include "config.h"
#include "ha.h"

#define SHAMAN_BIN	"/usr/sbin/shaman"

#ifndef GFS2_MAGIC
#define GFS2_MAGIC		0x01161970
#endif

#ifndef NFS_SUPER_MAGIC
#define NFS_SUPER_MAGIC		0x6969
#endif

/* Kernel sources, fs/fuse/inode.c */
#ifndef FUSE_SUPER_MAGIC
#define FUSE_SUPER_MAGIC	0x65735546
#endif

#ifndef EXT4_SUPER_MAGIC
#define EXT4_SUPER_MAGIC	0xEF53
#endif

static int is_shared_fs(const char *path)
{
	struct statfs st;

	if (statfs(path, &st)) {
		logger(-1, errno, "statfs '%s'", path);
		return -1;
	}

	return (st.f_type == GFS2_MAGIC ||
		st.f_type == NFS_SUPER_MAGIC ||
		st.f_type == FUSE_SUPER_MAGIC);
}

/**
 * Check whether shaman binary is present in the standard path
 *
 * @retval 0 on success
 * @retval -1 if shaman binary is not present
 * @retval >0 if error is occured in the process of checking
 */
static int is_shaman_present(void)
{
	return stat_file(SHAMAN_BIN);
}

static void shaman_get_resname(int veid, char *buf, int size)
{
	snprintf(buf, size, "ct-%d", veid);
}

int handle_set_cmd_on_ha_cluster(int veid, const char *ve_private,
		struct ha_params *cmdline,
		struct ha_params *config,
		int save)
{
	int i = 5;
	struct stat st;
	char command[NAME_MAX];
	char resname[NAME_MAX];
	char *argv[] = {SHAMAN_BIN, "-i", "-q", command, resname, NULL, NULL, NULL};
	char prio[NAME_MAX];

	if (cmdline->ha_enable) {
		/*
		 * If there is a '--ha-enable yes' in the command line, then use 'add'
		 * command to create resource file and set up needed parameters.
		 */
		snprintf(command, sizeof(command), "%s",
				(cmdline->ha_enable == YES) ? "add" : "del");
	} else if (cmdline->ha_prio) {
		snprintf(command, sizeof(command), "set");
	} else {
		/* HA options are not present in the command line */
		return 0;
	}

	/*
	 * Check that '--ha-*' options are always used along with '--save', otherwise
	 * CT config and HA cluster contents could become unsynchronized.
	 */
	if (!save) {
		logger(-1, 0, "High Availability Cluster options could be set only"
			" when --save option is provided");
		return VZ_NOT_ENOUGH_PARAMS;
	}

	/*
	 * If HA_ENABLE is explicitely set to "no" in the CT config and the command
	 * line doesn't provide the '--ha_enable' option -- just save parameters
	 * in the CT config and don't run shaman.
	 */
	if ((config->ha_enable == NO) && !cmdline->ha_enable)
		return 0;

	if (stat(ve_private, &st) || !is_shared_fs(ve_private))
		return 0;

	if (!is_shaman_present())
		return 0;

	shaman_get_resname(veid, resname, sizeof(resname));

	if (cmdline->ha_prio || config->ha_prio) {
		snprintf(prio, sizeof(prio), "%lu",
				 cmdline->ha_prio ? *cmdline->ha_prio : *config->ha_prio);
		argv[i++] = "--prio";
		argv[i++] = prio;
	}

	return run_script(argv, NULL, 0);
}
