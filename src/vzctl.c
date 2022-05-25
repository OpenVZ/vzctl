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
 *
 * Config parsing and checking functions for different actions,
 * action functions itself, list routines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <getopt.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/param.h>

#include <vzctl/libvzctl.h>

#include "vzctl.h"
#include "config.h"
#include "script.h"
#include "util.h"
#include "ha.h"

#define VZCTL_VEID_MAX	2147483644

static int use_args_from_env;

volatile sig_atomic_t child_exited;
/* function prototypes */

int handle_set_cmd_on_ha_cluster(int veid, const char *ve_private,
		struct ha_params *cmdline,
		struct ha_params *config,
		int save);
struct CParam *gparam;
extern struct CParam tmpparam;

static void cleanup_callback(int sig)
{
	fprintf(stderr, "Cancelling...\n");
	vzctl2_cancel_last_operation();
}

struct option_alternative {
	const char *long_name;
	int short_name;
};

static struct option set_options[] =
{
	{"save",	no_argument, NULL, PARAM_SAVE},
	{"vetype", required_argument, NULL, PARAM_VETYPE},
	{"ext_ipadd",	required_argument, NULL, PARAM_EXT_IP_ADD},
	{"ext_ipdel",	required_argument, NULL, PARAM_EXT_IP_DEL},
	{"skip_arpdetect",no_argument, NULL, PARAM_SKIP_ARPDETECT},
	{"userpasswd",	required_argument, NULL, PARAM_USERPW},
	{"noatime",	required_argument, NULL, PARAM_NOATIME},
	{"crypted",	no_argument, NULL, PARAM_PW_CRYPTED},
	{"reset_ub",	no_argument, NULL, PARAM_RESETUB},
	{"mount_opts", required_argument, NULL, PARAM_MOUNT_OPTS},
	{"apply_iponly", required_argument, NULL, PARAM_APPLY_IPONLY},
		{"apply-iponly", required_argument, NULL, PARAM_APPLY_IPONLY},
	{"setmode",	required_argument, NULL, PARAM_SETMODE},
	{"ostemplate", required_argument, NULL, PARAM_OSTEMPLATE},
	{"distribution", required_argument, NULL, PARAM_DISTRIBUTION},

/* --device-add hdd --size <N> --type <expanded|raw> --image <path> */
	{"device-add", required_argument, NULL, PARAM_DEVICE_ADD},
	  {"size", required_argument, NULL, PARAM_SIZE},
	  {"image", required_argument, NULL, PARAM_IMAGE},
	  {"device", required_argument, NULL, PARAM_DEVICE},
	  {"device-uuid", required_argument, NULL, PARAM_DEVICE_UUID},
	  {"mnt", required_argument, NULL, PARAM_DISK_MNT},
	  {"offline", no_argument, NULL, PARAM_DISK_OFFLINE},
	  {"storage-url", required_argument, NULL, PARAM_DISK_STORAGE_URL},
	  {"encryption-keyid",required_argument, NULL, PARAM_ENC_KEYID},
	  {"reencrypt",no_argument, NULL, PARAM_ENC_REENCRYPT},
	  {"wipe",no_argument, NULL, PARAM_ENC_WIPE},
	  {"recreate",no_argument, NULL, PARAM_RECREATE},
	{"enable", no_argument, NULL, PARAM_ENABLE},
	{"disable", no_argument, NULL, PARAM_DISABLE},
	{"device-del", required_argument, NULL, PARAM_DEVICE_DEL},
	  {"detach-only", no_argument, NULL, PARAM_DISK_DETACH},
	  {"detach", no_argument, NULL, PARAM_DISK_DETACH},
	{"device-set", required_argument, NULL, PARAM_DEVICE_SET},
	{"applyconfig", required_argument, NULL, PARAM_APPCONF},
	{"applyconfig_map", required_argument, NULL, PARAM_APPCONF_MAP},
/* VETH */
	{"netif_add",	required_argument, NULL, VZCTL_PARAM_NETIF_ADD},
	{"netif_del",	required_argument, NULL, VZCTL_PARAM_NETIF_DEL},
	{"netif_mac_renew",	no_argument, NULL, PARAM_NETIF_MAC_RENEW},
	{"default_gateway",	required_argument, NULL, PARAM_DEF_GW},
	{"default_gw",	required_argument, NULL, PARAM_DEF_GW},
	{"ifname",	required_argument, NULL, VZCTL_PARAM_NETIF_IFNAME},
	{"mac",		required_argument, NULL, VZCTL_PARAM_NETIF_MAC},
	{"host_mac",	required_argument, NULL, VZCTL_PARAM_NETIF_HOST_MAC},
	{"host_ifname",	required_argument, NULL, VZCTL_PARAM_NETIF_HOST_IFNAME},
	{"gateway",	required_argument, NULL, VZCTL_PARAM_NETIF_GW},
		{"gw",		required_argument, NULL, VZCTL_PARAM_NETIF_GW},
	{"gateway6",	required_argument, NULL, VZCTL_PARAM_NETIF_GW6},
		{"gw6",		required_argument, NULL, VZCTL_PARAM_NETIF_GW6},
	{"network",	required_argument, NULL, VZCTL_PARAM_NETIF_NETWORK},
	{"dhcp",	required_argument, NULL, VZCTL_PARAM_NETIF_DHCP},
	{"dhcp6",	required_argument, NULL, VZCTL_PARAM_NETIF_DHCP6},
	{"mac_filter",	required_argument, NULL, VZCTL_PARAM_NETIF_MAC_FILTER},
	{"configure",	required_argument, NULL, VZCTL_PARAM_NETIF_CONFIGURE_MODE},
	{"nettype",     required_argument, NULL, VZCTL_PARAM_NETIF_NETTYPE},
	{"vporttype",   required_argument, NULL, VZCTL_PARAM_NETIF_VPORT_TYPE},

/*	bindmount	*/
	{"bindmount_add",	required_argument, NULL, VZCTL_PARAM_BINDMOUNT},
		{"bindmount-add",	required_argument, NULL, VZCTL_PARAM_BINDMOUNT},
	{"bindmount_del",	required_argument, NULL, VZCTL_PARAM_BINDMOUNT_DEL},
		{"bindmount-del",	required_argument, NULL, VZCTL_PARAM_BINDMOUNT_DEL},
	/**
	 * processed by vzctl_add_env_param_by_id
	 **/
	{"onboot",	required_argument, NULL, VZCTL_PARAM_ONBOOT},
	{"autostopt",	required_argument, NULL, VZCTL_PARAM_AUTOSTOP},
	{"root",	required_argument, NULL, VZCTL_PARAM_VE_ROOT},
	{"private",	required_argument, NULL, VZCTL_PARAM_VE_PRIVATE},
	{"ip",		required_argument, NULL, PARAM_IP_ADD},
	{"ipadd",	required_argument, NULL, PARAM_IP_ADD},
	{"ipdel",	required_argument, NULL, PARAM_IP_DEL},
/* Disk quota parameters */
	{"diskspace",	required_argument, NULL, PARAM_SIZE},
	{"diskinodes",	required_argument, NULL, VZCTL_PARAM_DISKINODES},
	{"quotatime",	required_argument, NULL, VZCTL_PARAM_QUOTATIME},
	{"quotaugidlimit",required_argument,NULL,VZCTL_PARAM_QUOTAUGIDLIMIT},
	{"journalled_quota",	required_argument, NULL, VZCTL_PARAM_JOURNALED_QUOTA},
		{"jquota",	required_argument, NULL, VZCTL_PARAM_JOURNALED_QUOTA},
	{"slmmemorylimit", required_argument,NULL,VZCTL_PARAM_SLMMEMORYLIMIT},
	{"slmmode",	required_argument,NULL,	VZCTL_PARAM_SLMMODE},
/* Host parameters */
	{"description",	required_argument, NULL, VZCTL_PARAM_DESCRIPTION},
	{"name",	required_argument, NULL, PARAM_NAME},
	{"hostname",	required_argument, NULL, VZCTL_PARAM_HOSTNAME},
	{"nameserver",	required_argument, NULL, VZCTL_PARAM_NAMESERVER},
	{"searchdomain",required_argument, NULL, VZCTL_PARAM_SEARCHDOMAIN},
/* Rate */
	{"rate",	required_argument, NULL, VZCTL_PARAM_RATE},
	{"ratebound",	required_argument, NULL, VZCTL_PARAM_RATEBOUND},
/* User beancounter parameters */
	{"kmemsize",	required_argument, NULL, VZCTL_PARAM_KMEMSIZE},
	{"lockedpages",	required_argument, NULL, VZCTL_PARAM_LOCKEDPAGES},
	{"privvmpages",	required_argument, NULL, VZCTL_PARAM_PRIVVMPAGES},
	{"shmpages",	required_argument, NULL, VZCTL_PARAM_SHMPAGES},
	{"ipcshmpages",	required_argument, NULL, VZCTL_PARAM_IPCSHMPAGES},
	{"anonshpages", required_argument, NULL, VZCTL_PARAM_ANONSHPAGES},
	{"numproc",	required_argument, NULL, VZCTL_PARAM_NUMPROC},
	{"physpages",	required_argument, NULL, VZCTL_PARAM_PHYSPAGES},
	{"rsspages",	required_argument, NULL, VZCTL_PARAM_RSSPAGES},
	{"vmguarpages",	required_argument, NULL, VZCTL_PARAM_VMGUARPAGES},
	{"oomguarpages",required_argument, NULL, VZCTL_PARAM_OOMGUARPAGES},
	{"oomguar",	required_argument, NULL, VZCTL_PARAM_OOMGUAR},
	{"numtcpsock",	required_argument, NULL, VZCTL_PARAM_NUMTCPSOCK},
	{"numsock",	required_argument, NULL, VZCTL_PARAM_NUMSOCK},
	{"numflock",	required_argument, NULL, VZCTL_PARAM_NUMFLOCK},
	{"numpty",	required_argument, NULL, VZCTL_PARAM_NUMPTY},
	{"numsiginfo",	required_argument, NULL, VZCTL_PARAM_NUMSIGINFO},
	{"tcpsndbuf",	required_argument, NULL, VZCTL_PARAM_TCPSNDBUF},
	{"tcpsendbuf",	required_argument, NULL, VZCTL_PARAM_TCPSNDBUF},
	{"tcprcvbuf",	required_argument, NULL, VZCTL_PARAM_TCPRCVBUF},
	{"othersockbuf",required_argument, NULL, VZCTL_PARAM_OTHERSOCKBUF},
	{"unixsockbuf",	required_argument, NULL, VZCTL_PARAM_UNIXSOCKBUF},
	{"dgramrcvbuf",	required_argument, NULL, VZCTL_PARAM_DGRAMRCVBUF},
	{"sockrcvbuf",	required_argument, NULL, VZCTL_PARAM_SOCKRCVBUF},
	{"numothersock",required_argument, NULL, VZCTL_PARAM_NUMOTHERSOCK},
	{"numunixsock",	required_argument, NULL, VZCTL_PARAM_NUMUNIXSOCK},
	{"numfile",	required_argument, NULL, VZCTL_PARAM_NUMFILE},
	{"dcachesize",	required_argument, NULL, VZCTL_PARAM_DCACHESIZE},
	{"dcache",	required_argument, NULL, VZCTL_PARAM_DCACHESIZE},
	{"numiptent",	required_argument, NULL, VZCTL_PARAM_NUMIPTENT},
	{"iptentries",  required_argument, NULL, VZCTL_PARAM_NUMIPTENT},
	{"avnumproc",	required_argument, NULL, VZCTL_PARAM_AVNUMPROC},
	{"swappages",	required_argument, NULL, VZCTL_PARAM_SWAPPAGES},
	{"swap",	required_argument, NULL, VZCTL_PARAM_SWAP},
	{"totvmpages",	required_argument, NULL, VZCTL_PARAM_TOTVMPAGES},
	{"memory",	required_argument, NULL, VZCTL_PARAM_MEMORY},
		{"ram", required_argument, NULL, VZCTL_PARAM_MEMORY},
	{"memguarantee",required_argument, NULL, VZCTL_PARAM_MEM_GUARANTEE},
	{"vm_overcommit",  required_argument, NULL, VZCTL_PARAM_VM_OVERCOMMIT},
	{"pagecache-isolation",  required_argument, NULL, VZCTL_PARAM_PAGECACHE_ISOLATION},
	{"num-memory-subgroups",  required_argument, NULL, VZCTL_PARAM_NUMMEMORYSUBGROUPS},
	{"numnetif",  required_argument, NULL, VZCTL_PARAM_NUMNETIF},
/* CPU */
	{"cpuweight",	required_argument, NULL, VZCTL_PARAM_CPUWEIGHT},
	{"cpulimit",	required_argument, NULL, VZCTL_PARAM_CPULIMIT},
	{"cpuunits",	required_argument, NULL, VZCTL_PARAM_CPUUNITS},
	{"burst_cpu_avg_usage",	required_argument, NULL, VZCTL_PARAM_BURST_CPU_AVG_USAGE,},
	{"burst_cpulimit", required_argument, NULL, VZCTL_PARAM_BURST_CPULIMIT,},

	{"capability",	required_argument, NULL, PARAM_CAP},
/*	Devices	*/
	{"devices",	required_argument, NULL, VZCTL_PARAM_DEVICES},
	{"devnodes",	required_argument, NULL, VZCTL_PARAM_DEVNODES},
	{"config_customized", required_argument, NULL, VZCTL_PARAM_CONFIG_CUSTOMIZE},
	{"origin_sample", required_argument, NULL, VZCTL_PARAM_CONFIG_SAMPLE},

	{"iptables",	required_argument, NULL, VZCTL_PARAM_IPTABLES},
	{"netfilter",	required_argument, NULL, VZCTL_PARAM_NETFILTER},
	{"netdev_add",	required_argument, NULL, VZCTL_PARAM_NETDEV},
		{"netdev-add",	required_argument, NULL, VZCTL_PARAM_NETDEV},
	{"netdev_del",	required_argument, NULL, VZCTL_PARAM_NETDEV_DEL},
		{"netdev-del",	required_argument, NULL, VZCTL_PARAM_NETDEV_DEL},
	{"disabled",	required_argument, NULL, VZCTL_PARAM_DISABLED},
	{"meminfo",	required_argument, NULL, VZCTL_PARAM_MEMINFO},
	{"cpus",	required_argument, NULL, VZCTL_PARAM_CPUS},
	{"cpumask",	required_argument, NULL, VZCTL_PARAM_CPUMASK},
	{"nodemask",	required_argument, NULL, VZCTL_PARAM_NODEMASK},
	{"ioprio",	required_argument, NULL, VZCTL_PARAM_IOPRIO},
	{"iolimit",	required_argument, NULL, VZCTL_PARAM_IOLIMIT},
	{"iopslimit",	required_argument, NULL, VZCTL_PARAM_IOPSLIMIT},
	{"features",	required_argument, NULL, VZCTL_PARAM_FEATURES},
	{"bootorder",	required_argument, NULL, VZCTL_PARAM_BOOTORDER},
	{"pci_add",     required_argument, NULL, VZCTL_PARAM_PCI},
		{"pci-add",     required_argument, NULL, VZCTL_PARAM_PCI},
	{"pci_del",     required_argument, NULL, VZCTL_PARAM_PCI_DEL},
		{"pci-del",     required_argument, NULL, VZCTL_PARAM_PCI_DEL},

/* High Availability Cluster */
	{"ha_enable", required_argument, NULL, VZCTL_PARAM_HA_ENABLE},
		{"ha-enable", required_argument, NULL, VZCTL_PARAM_HA_ENABLE},
		{"ha", required_argument, NULL, VZCTL_PARAM_HA_ENABLE},
	{"ha_prio", required_argument, NULL, VZCTL_PARAM_HA_PRIO},
		{"ha-prio", required_argument, NULL, VZCTL_PARAM_HA_PRIO},

/* Legacy parameters left for command line compatibility */
	{"offline_management",  required_argument, NULL, PARAM_DUMMY},
	{"offline_service", required_argument, NULL, PARAM_DUMMY},

	{ NULL, 0, NULL, 0 }
};

static struct option create_options[] =
{
	{"ve_layout", required_argument, NULL, PARAM_VE_LAYOUT},
		{"layout", required_argument, NULL, PARAM_VE_LAYOUT},
	{"ostemplate", required_argument, NULL, PARAM_OSTEMPLATE},
	{"vetype", required_argument, NULL, PARAM_VETYPE},
	{"config", required_argument, NULL, PARAM_CONFIG},
	{"private",required_argument, NULL, VZCTL_PARAM_VE_PRIVATE},
	{"root",   required_argument, NULL, VZCTL_PARAM_VE_ROOT},
	{"ipadd",  required_argument, NULL, VZCTL_PARAM_IP_ADDRESS},
	{"hostname",	required_argument, NULL, VZCTL_PARAM_HOSTNAME},
	{"skip_app_templates",no_argument, NULL, PARAM_SKIP_APP},
	{"name",	required_argument, NULL, PARAM_NAME},
	{"description",	required_argument, NULL, VZCTL_PARAM_DESCRIPTION},
	{"expanded",	no_argument, NULL, PARAM_EXPANDED},
	{"preallocated",no_argument, NULL, PARAM_PREALLOCATED},
		{"plain",no_argument, NULL, PARAM_PREALLOCATED},
	{"raw"		,no_argument, NULL, PARAM_RAW},
	{"mount_opts", required_argument, NULL, PARAM_MOUNT_OPTS},
	{"id",	required_argument, NULL, PARAM_GUID},
		{"uuid", required_argument, NULL, PARAM_GUID},
	{"force", no_argument, NULL, PARAM_FORCE},
	{"diskspace",	required_argument, NULL, VZCTL_PARAM_DISKSPACE},
	{"diskinodes",	required_argument, NULL, VZCTL_PARAM_DISKINODES},
	{"no-hdd",      no_argument, NULL, PARAM_NO_HDD},
	{"encryption-keyid",required_argument, NULL, PARAM_ENC_KEYID},

	{ NULL, 0, NULL, 0 }
};

static struct option convert_options[] =
{
	{"config", required_argument, NULL, PARAM_CONFIG},
	{"ve_layout", required_argument, NULL, PARAM_VE_LAYOUT},
	{ NULL, 0, NULL, 0 }
};

static struct option stop_options[] =
{
	{"fast", no_argument, NULL, PARAM_FAST},
	{"force", no_argument, NULL, PARAM_FORCE},
	{"skip-umount", no_argument, NULL, PARAM_SKIP_UMOUNT},
	{ NULL, 0, NULL, 0 }
};

static struct option start_options[] =
{
	{"force", no_argument, NULL, PARAM_FORCE},
	{"skip_ve_setup",no_argument, NULL, PARAM_SKIP_VE_SETUP},
	{"wait",no_argument, NULL, PARAM_WAIT},
	{"osrelease",	required_argument, NULL, PARAM_OSRELEASE},
	{"skip-fsck",	no_argument, NULL, PARAM_SKIP_FSCK},
	{"cloud-init",	required_argument, NULL, PARAM_CLOUD_INIT},
	{"repair",	no_argument, NULL, PARAM_REPAIR},
	{ NULL, 0, NULL, 0 }
};

static struct option chkpnt_options[] =
{
	/*      sub commands    */
	{"dump",	no_argument, NULL, PARAM_DUMP},
	{"suspend",	no_argument, NULL, PARAM_SUSPEND},
	{"resume",	no_argument, NULL, PARAM_RESUME},
	{"kill",	no_argument, NULL, PARAM_KILL},
	/*      flags		*/
	{"flags",	required_argument, NULL, PARAM_CPU_FLAGS},
	{"context",	required_argument, NULL, PARAM_CPTCONTEXT},
	{"dumpfile",	required_argument, NULL, PARAM_DUMPFILE},
	{"skip_arpdetect",	no_argument, NULL, PARAM_SKIP_ARPDETECT},
	{"keep_pages",	no_argument, NULL, PARAM_KEEP_PAGES},
	{"unfreeze",	no_argument, NULL, PARAM_UNFREEZE},
	{"stop-tracker",no_argument, NULL, PARAM_CPT_STOP_TRACKER},
	{"create-devmap",no_argument, NULL, PARAM_CPT_CREATE_DEVMAP},
	{ NULL, 0, NULL, 0 }
};

static struct option restore_options[] =
{
	/*      sub commands    */
	{"undump",	no_argument, NULL, PARAM_UNDUMP},
	{"kill",	no_argument, NULL, PARAM_KILL},
	{"resume",	no_argument, NULL, PARAM_RESUME},
	/*      flags		*/
	{"dumpfile",	required_argument, NULL, PARAM_DUMPFILE},
	{"flags",	required_argument, NULL, PARAM_CPU_FLAGS},
	{"context",	required_argument, NULL, PARAM_CPTCONTEXT},
	{"skip_arpdetect",	no_argument, NULL, PARAM_SKIP_ARPDETECT},
	{"skip-fsck",	no_argument, NULL, PARAM_SKIP_FSCK},
	{ NULL, 0, NULL, 0 }
};

static struct option snapshot_options[] =
{
	{"id",	required_argument, NULL, PARAM_SNAPSHOT_GUID},
		{"uuid", required_argument, NULL, PARAM_SNAPSHOT_GUID},
	{"name", required_argument, NULL, PARAM_SNAPSHOT_NAME},
	{"description", required_argument, NULL, PARAM_SNAPSHOT_DESC},
	{"skip_dump", no_argument, NULL, PARAM_SNAPSHOT_SKIP_DUMP},
		{"skip-dump", no_argument, NULL, PARAM_SNAPSHOT_SKIP_DUMP},
		{"skip_suspend", no_argument, NULL, PARAM_SNAPSHOT_SKIP_DUMP},
		{"skip-suspend", no_argument, NULL, PARAM_SNAPSHOT_SKIP_DUMP},
	{"component-name", required_argument, NULL, PARAM_SNAPSHOT_COMPONENT_NAME},
	{"env", no_argument, NULL, PARAM_ENV_ARGS},
	{ NULL, 0, NULL, 0 }
};

static struct option snapshot_switch_options[] =
{
	{"id",	required_argument, NULL, PARAM_SNAPSHOT_GUID},
		{"uuid", required_argument, NULL, PARAM_SNAPSHOT_GUID},
	{"skip-resume", no_argument, NULL, PARAM_SKIP_RESUME},
	{ NULL, 0, NULL, 0 }
};

static struct option snapshot_delete_options[] =
{
	{"id",	required_argument, NULL, PARAM_SNAPSHOT_GUID},
		{"uuid", required_argument, NULL, PARAM_SNAPSHOT_GUID},
	{ NULL, 0, NULL, 0 }
};

static struct option snapshot_mount_options[] =
{
	{"id",	required_argument, NULL, PARAM_SNAPSHOT_GUID},
		{"uuid", required_argument, NULL, PARAM_SNAPSHOT_GUID},
	{"target", required_argument, NULL, PARAM_TARGET_DIR},
	{ NULL, 0, NULL, 0 }
};

static struct option snapshot_umount_options[] =
{
	{"id",	required_argument, NULL, PARAM_SNAPSHOT_GUID},
		{"uuid", required_argument, NULL, PARAM_SNAPSHOT_GUID},
	{ NULL, 0, NULL, 0 }
};

static struct option del_options[] =
{
	{"safe", no_argument, NULL, PARAM_SAFE},
	{ NULL, 0, NULL, 0 }
};

static struct option mount_options[] =
{
	{"skip_ve_setup",no_argument, NULL, PARAM_SKIP_VE_SETUP},
	{"skipquotacheck", no_argument, NULL, PARAM_SKIPQUOTACHECK},
	{"skip-fsck",	no_argument, NULL, PARAM_SKIP_FSCK},
	{ NULL, 0, NULL, 0 }
};

static struct option reinstall_options[] =
{
	{"skipbackup", no_argument, NULL, PARAM_SKIPBACKUP},
	{"skipscripts", no_argument, NULL, PARAM_SKIPSCRIPTS},
	{"scripts", required_argument, NULL, PARAM_REINSTALL_SCRIPTS},
	{"resetpwdb", no_argument, NULL, PARAM_RESETPWDB},
	{"listscripts", no_argument, NULL, PARAM_REINSTALL_LIST},
	{"desc", no_argument, NULL, PARAM_REINSTALL_DESC},
	{"vzpkg_opts", required_argument, NULL, PARAM_VZPKG_OPTS},
	{"ostemplate", required_argument, NULL, PARAM_OSTEMPLATE},
	{ NULL, 0, NULL, 0}
};

static struct option register_options[] =
{
	{"skip_cluster", no_argument, NULL, PARAM_SKIP_CLUSTER},
	{"force",	no_argument, NULL, PARAM_FORCE},
	{"renew",	no_argument, NULL, PARAM_RENEW},
	{"start",	no_argument, NULL, PARAM_REG_START},
	{"id",  required_argument, NULL, PARAM_GUID},
		{"uuid", required_argument, NULL, PARAM_GUID},
	{"name",	required_argument, NULL, PARAM_NAME},

	{ NULL, 0, NULL, 0 }
};

static struct option console_options[] =
{
	{"start", no_argument, NULL, 's'},
	{ NULL, 0, NULL, 0 }
};

struct option_alternative option_alternatives[] =
{
	{"numproc",     'p'},
	{"kmemsize",    'k'},
	{"tcprcvbuf",   'b'},
	{"lockedpages", 'l'},
	{"numfile",     'n'},
	{"numflock",    'f'},
	{"numpty",      't'},
	{"numsiginfo",  'i'},
	{"dcachesize",  'x'},
	{"numiptent",   'e'}
};

static struct option compact_options[] =
{
	{"defrag",	no_argument, NULL, PARAM_COMPACT_DEFRAG},
	{"dry",	no_argument, NULL, PARAM_COMPACT_DRY},
	{"threshold",	required_argument, NULL, PARAM_COMPACT_THRESHOLD},
	{"delta",	required_argument, NULL, PARAM_COMPACT_DELTA},
	{ NULL, 0, NULL, 0 }
};


static int check_argv_tail(int argc, char **argv)
{
	if (optind < argc) {
		printf("non-option ARGV-elements: ");
		while (optind < argc)
			printf ("%s ", argv[optind++]);
		printf("\n");
		return VZ_INVALID_PARAMETER_SYNTAX;
	}
	return 0;
}

int ParseCreateOptions(ctid_t ctid, struct CParam *param, int argc, char **argv)
{
	int c, ret = 0;
	int ploop_type = -1;

	while (1) {
		int option_index = -1;

		c = getopt_long(argc, argv, "", create_options, &option_index);
		if (c == -1)
			break;
		if (c == '?')
			return VZ_INVALID_PARAMETER_VALUE;

		if (c == PARAM_VE_LAYOUT) {
			if (!strcmp(optarg, "ploop"))
				gparam->ve_layout = VZCTL_LAYOUT_5;
			else if (parse_int(optarg, &tmpparam.ve_layout))
				return VZ_INVALID_PARAMETER_SYNTAX;
			if (gparam->ve_layout != VZCTL_LAYOUT_5)
				return VZ_INVALID_PARAMETER_SYNTAX;
		} else if (c == PARAM_OSTEMPLATE) {
			gparam->ostmpl = strdup(optarg);
		} else if (c == PARAM_SKIP_APP) {
			gparam->skip_app_templates = 1;
		} else if (c == PARAM_CONFIG) {
			gparam->config = strdup(optarg);
		} else if (c == PARAM_NAME) {
			gparam->name = strdup(optarg);
		} else if (c == PARAM_EXPANDED) {
			ploop_type = PLOOP_EXPANDED_MODE;
		} else if (c == PARAM_PREALLOCATED) {
			ploop_type = PLOOP_EXPANDED_PREALLOCATED_MODE;
		} else if (c == PARAM_RAW) {
			ploop_type = PLOOP_RAW_MODE;
		} else if (c == PARAM_FORCE) {
			gparam->force = 1;
		} else if (c == PARAM_NO_HDD) {
			gparam->no_hdd = 1;
		} else if (c == PARAM_ENC_KEYID) {
			gparam->enc_keyid = optarg;
		} else if (c == PARAM_GUID) {
                        ctid_t ctid;
			if (vzctl2_parse_ctid(optarg, ctid)) {
				fprintf(stderr, "Invalid guid is specified: %s\n",
						optarg);
				return VZ_INVALID_PARAMETER_SYNTAX;
			}
			gparam->guid = strdup(ctid);
		} else if (c >= VZCTL_PARAM_KMEMSIZE) {
			ret = vzctl_add_env_param_by_id(ctid, c, optarg);
			if (ret) {
				if (option_index < 0)
					return vzctl_err(ret, 0, "Bad parameter for"
							" -%c: %s", c, optarg);
				else
					return vzctl_err(ret, 0, "Bad parameter for"
							" --%s: %s", create_options[option_index].name, optarg);
			}
		} else {
			ret = VZ_INVALID_PARAMETER_SYNTAX;
			if (option_index < 0)
				return vzctl_err(ret, 0, "Bad parameter for -%c: %s",
						c, optarg);
			return vzctl_err(ret, 0, "Bad parameter for --%s: %s",
					create_options[option_index].name, optarg);
		}
	}

	if (ploop_type != -1)
		gparam->ploop_type = ploop_type;

	return check_argv_tail(argc, argv);
}

int ParseStopOptions(struct CParam *param, int argc, char **argv)
{
	int c, ret = 0;

	while (1)
	{
		c = getopt_long (argc, argv, "", stop_options, NULL);
		if (c == -1)
			break;
		switch (c)
		{
			case PARAM_FAST	:
				gparam->fastkill = 1;
				break;
			case PARAM_FORCE	:
				gparam->stop_flags |= VZCTL_FORCE;
				break;
			case PARAM_SKIP_UMOUNT:
				gparam->stop_flags |= VZCTL_SKIP_UMOUNT;
				break;
			default		:
				ret = VZ_INVALID_PARAMETER_SYNTAX;
				break;
		}
	}
	return ret;
}

int ParseStartOptions(struct CParam *param, int argc, char **argv)
{
	int c, ret = 0;

	while (1)
	{
		c = getopt_long (argc, argv, "", start_options, NULL);
		if (c == -1)
			break;
		if (c == '?')
			return VZ_INVALID_PARAMETER_VALUE;
		switch (c)
		{
			case PARAM_FORCE	:
				gparam->start_force = YES;
				break;
			case PARAM_SKIP_VE_SETUP:
				gparam->skip_ve_setup = YES;
				break;
			case PARAM_WAIT:
				gparam->wait = YES;
				break;
			case PARAM_OSRELEASE:
				gparam->osrelease = strdup(optarg);
				break;
			case PARAM_CLOUD_INIT:
				gparam->cidata_fname = optarg;
				break;
			case PARAM_SKIP_FSCK:
				gparam->skip_fsck = 1;
				break;
			case PARAM_REPAIR:
				gparam->flags |= VZCTL_ENV_START_REPAIR;
				break;
			default	:
				ret = VZ_INVALID_PARAMETER_SYNTAX;
				break;
		}
	}
	ret = check_argv_tail(argc, argv);
	return ret;
}

int ParseMigrationOptions(struct CParam *param, struct option *opt,
		int argc, char **argv)
{
	int c, ret = 0;

	while (1)
	{
		c = getopt_long (argc, argv, "", opt, NULL);
		if (c == -1)
			break;
		if (c == '?')
			return VZ_INVALID_PARAMETER_VALUE;
		switch (c)
		{
			case PARAM_DUMPFILE	:
				gparam->dumpfile = strdup(optarg);
				break;
			case PARAM_CPTCONTEXT	:
				gparam->cptcontext = strtoul(optarg, NULL, 16);
				break;
			case PARAM_CPU_FLAGS	:
				gparam->cpu_flags = strtoul(optarg, NULL, 0);
				break;
			case PARAM_KILL:
				gparam->cpt_cmd = VZCTL_CMD_KILL;
				break;
			case PARAM_UNDUMP:
				gparam->cpt_cmd = VZCTL_CMD_UNDUMP;
				break;
			case PARAM_DUMP:
				gparam->cpt_cmd = VZCTL_CMD_DUMP;
				break;
			case PARAM_RESUME:
				gparam->cpt_cmd = VZCTL_CMD_RESUME;
				break;
			case PARAM_SUSPEND:
				gparam->cpt_cmd = VZCTL_CMD_SUSPEND;
				break;
			case PARAM_SKIP_ARPDETECT:
				gparam->skip_arpdetect = YES;
				break;
			case PARAM_SKIP_FSCK:
				gparam->skip_fsck = 1;
				break;
			case PARAM_KEEP_PAGES:
				gparam->cpt_flags |= VZCTL_CPT_KEEP_PAGES;
				break;
			case PARAM_UNFREEZE:
				gparam->cpt_flags |= VZCTL_CPT_UNFREEZE_ON_DUMP;
				break;
			case PARAM_CPT_STOP_TRACKER:
				gparam->cpt_flags |= VZCTL_CPT_STOP_TRACKER;
				break;
			case PARAM_CPT_CREATE_DEVMAP:
				gparam->cpt_flags |= VZCTL_CPT_CREATE_DEVMAP;
				break;
			default		:
				ret = VZ_INVALID_PARAMETER_SYNTAX;
				break;
		}
	}

	if ((gparam->cpt_flags & VZCTL_CPT_STOP_TRACKER) && gparam->cpt_cmd != VZCTL_CMD_SUSPEND) {
		fprintf(stderr, "The --stop-tracker option have to be used with --suspend only\n");
		return VZ_INVALID_PARAMETER_SYNTAX;
	}

	ret = check_argv_tail(argc, argv);
	return ret;
}

static int ParseSnapshotOptions(struct CParam *param, struct option *opt,
	int argc, char **argv)
{
	int c, ret = 0;

	while (1)
	{
		c = getopt_long (argc, argv, "", opt, NULL);
		if (c == -1)
			break;
		if (c == '?')
			return VZ_INVALID_PARAMETER_VALUE;
		switch (c)
		{
		case PARAM_ENV_ARGS:
			use_args_from_env = 1;
			break;
		case PARAM_SNAPSHOT_GUID: {
			char guid[64];

			if (vzctl_get_normalized_guid(optarg, guid, sizeof(guid))) {
				fprintf(stderr, "Invalid guid is specified: %s\n",
						optarg);
				return VZ_INVALID_PARAMETER_SYNTAX;
			}
			param->snapshot_guid = strdup(guid);
			break;
		}
		case PARAM_SNAPSHOT_NAME:
			param->snapshot_name = strdup(optarg);
			break;
		case PARAM_SNAPSHOT_DESC:
			param->snapshot_desc = strdup(optarg);
			break;
		case PARAM_SNAPSHOT_SKIP_DUMP:
			param->snapshot_flags = VZCTL_SNAPSHOT_SKIP_DUMP;
			break;
		case PARAM_TARGET_DIR:
			param->target_dir = strdup(optarg);
			break;
		case PARAM_SKIP_RESUME:
			param->snapshot_flags = VZCTL_SNAPSHOT_SKIP_RESUME;
			break;
		case PARAM_SNAPSHOT_COMPONENT_NAME:
			param->snapshot_component_name = strdup(optarg);;
			break;
		default		:
			ret = VZ_INVALID_PARAMETER_SYNTAX;
			break;
		}
	}
	ret = check_argv_tail(argc, argv);
	if (use_args_from_env) {
		char *data;
		if (param->snapshot_name == NULL &&
				(data = getenv(VZCTL_ENV_SNAPSHOT_NAME_PARAM)) != NULL)
			param->snapshot_name = strdup(data);
		if (param->snapshot_desc == NULL &&
				(data = getenv(VZCTL_ENV_SNAPSHOT_DESC_PARAM)) != NULL)
			param->snapshot_desc = strdup(data);
	}
	return ret;
}

int ParseDelOptions(struct CParam *param, int argc, char **argv)
{
	int c, ret = 0;

	while (1)
	{
		c = getopt_long (argc, argv, "", del_options, NULL);
		if (c == -1)
			break;
		switch (c)
		{
			case PARAM_SAFE	:
				param->del_safe = 1;
				break;
			default		:
				ret = VZ_INVALID_PARAMETER_SYNTAX;
				break;
		}
	}
	return ret;
}

int ParseMountOptions(struct CParam *param, int argc, char **argv)
{
	int c, ret = 0;

	while (1)
	{
		c = getopt_long (argc, argv, "", mount_options, NULL);
		if (c == -1)
			break;
		switch (c)
		{
			case PARAM_SKIP_VE_SETUP:
				gparam->skip_ve_setup = YES;
				break;
			case PARAM_SKIPQUOTACHECK	:
				break;
			case PARAM_SKIP_FSCK:
				gparam->skip_fsck = 1;
				break;
			default		:
				ret = VZ_INVALID_PARAMETER_SYNTAX;
				break;
		}
	}
	return ret;
}

int ParseConvertOptions(struct CParam *param, int argc, char **argv)
{
	int c, ret = 0;
	int val;

	while (1)
	{
		c = getopt_long (argc, argv, "", convert_options, NULL);
		if (c == -1)
			break;
		switch (c)
		{
			case PARAM_CONFIG	:
				param->config = strdup(optarg);
				break;
			case PARAM_VE_LAYOUT: {
				if (!strcmp(optarg, "ploop"))
					val = VZCTL_LAYOUT_5;
				else if (parse_int(optarg, &val)) {
					fprintf(stderr, "An incorrect value '%s' for --velayout is specified\n",
							optarg);
					return VZ_INVALID_PARAMETER_VALUE;
				}
				if (val != VZCTL_LAYOUT_5) {
					fprintf(stderr, "An incorrect value '%s' for --velayout is specified\n",
							optarg);
					return VZ_INVALID_PARAMETER_VALUE;
				}
				param->ve_layout = val;
				break;
			}
			default		:
				return VZ_INVALID_PARAMETER_SYNTAX;
		}
	}
	ret = check_argv_tail(argc, argv);
	return ret;
}

int ParseReinstallOptions(struct CParam *param, int argc, char **argv)
{
	int c, ret = 0;
	char *token;

	while (1)
	{
		c = getopt_long (argc, argv, "", reinstall_options, NULL);
		if (c == -1)
			break;
		if (c == '?')
			return VZ_INVALID_PARAMETER_VALUE;
		switch (c)
		{
			case PARAM_SKIPBACKUP	:
				param->skipbackup = YES;
				break;
			case PARAM_SKIPSCRIPTS	:
				param->skipscripts = YES;
				break;
			case PARAM_REINSTALL_LIST:
				param->reinstall_list = YES;
				break;
			case PARAM_REINSTALL_DESC:
				gparam->reinstall_desc = YES;
				break;
			case PARAM_REINSTALL_SCRIPTS: {
				char *buf = strdup(optarg);
				/* skip scripts for --scripts '' */
				if (optarg[0] == 0) {
					param->skipscripts = YES;
					break;
				}
				if ((token = strtok(buf, ",\t ")) == NULL)
					break;
				do {
					param->reinstall_scripts = ListAddElem(
						param->reinstall_scripts,
						token, NULL, NULL);
				} while ((token = strtok(NULL, ",\t ")));
				break;
			}
			case PARAM_VZPKG_OPTS: {
				char *buf = strdup(optarg);

				if ((token = strtok(buf, ",\t ")) == NULL)
					break;
				do {
					param->reinstall_opts = ListAddElem(
						param->reinstall_opts,
						token, NULL, NULL);
				} while ((token = strtok(NULL, ",\t ")));
				break;
			}
			case PARAM_OSTEMPLATE:
				param->ostmpl = strdup(optarg);
				break;
			case PARAM_RESETPWDB	:
				param->resetpwdb = YES;
				break;
			default		:
				ret = VZ_INVALID_PARAMETER_SYNTAX;
				break;
		}
	}
	ret = check_argv_tail(argc, argv);
	return ret;
}

int ParseConsoleOptions(int argc, char **argv, int *start, int *tty)
{
	int c, ret = 0;

	while (1) {
		c = getopt_long (argc, argv, "s", console_options, NULL);
		if (c == -1)
			break;
		if (c == '?')
			return VZ_INVALID_PARAMETER_VALUE;
		switch (c) {
		case 's':
			*start = 1;
			break;
		}
	}
	if (optind < argc) {
		if (parse_int(argv[optind], tty)) {
			fprintf(stderr, "Incorrect tty number '%s' is specified",
				argv[optind]);
			return VZ_INVALID_PARAMETER_VALUE;
		}
		argc--;
	}

	ret = check_argv_tail(argc, argv);
	return ret;
}

int ParseRegisterOptions(int argc, char **argv, int *flags, struct CParam *param)
{
	int c, ret = 0;

	while (1) {
		c = getopt_long (argc, argv, "", register_options, NULL);
		if (c == -1)
			break;
		if (c == '?')
			return VZ_INVALID_PARAMETER_VALUE;
		switch (c) {
		case PARAM_SKIP_CLUSTER	:
			*flags |= VZ_REG_SKIP_CLUSTER;
			break;
		case PARAM_FORCE	:
			*flags |= VZ_REG_FORCE;
			break;
		case PARAM_RENEW	:
			*flags |= VZ_REG_RENEW;
			break;
		case PARAM_REG_START:
			*flags |= VZ_REG_START;
			break;
		case PARAM_GUID: {
                        ctid_t ctid;
			if (vzctl2_parse_ctid(optarg, ctid)) {
				fprintf(stderr, "Invalid guid is specified: %s\n",
						optarg);
				return VZ_INVALID_PARAMETER_SYNTAX;
			}
			param->guid = strdup(ctid);
			break;
		}
		case PARAM_NAME:
			param->name = strdup(optarg);
			break;
		}
	}
	ret = check_argv_tail(argc, argv);
	return ret;
}

int ParseCompactOptions(struct CParam *param, int argc, char **argv)
{
	int c, ret = 0;

	while (1)
	{
		c = getopt_long (argc, argv, "", compact_options, NULL);
		if (c == -1)
			break;
		if (c == '?')
			return VZ_INVALID_PARAMETER_VALUE;
		switch (c)
		{
			case PARAM_COMPACT_DEFRAG	:
				gparam->compact_defrag = 1;
				break;
			case PARAM_COMPACT_DRY	:
				gparam->compact_dry = 1;
				break;
			case PARAM_COMPACT_THRESHOLD:
				if (parse_int(optarg, &gparam->compact_threshold)) {
					fprintf(stderr, "Incorrect threshold number '%s' is specified\n", optarg);
					return VZ_INVALID_PARAMETER_VALUE;
				}
				break;
			case PARAM_COMPACT_DELTA:
				if (parse_int(optarg, &gparam->compact_delta)) {
					fprintf(stderr, "Incorrect delta number '%s' is specified\n", optarg);
					return VZ_INVALID_PARAMETER_VALUE;
				}
				break;
			default	:
				ret = VZ_INVALID_PARAMETER_SYNTAX;
				break;
		}
	}
	ret = check_argv_tail(argc, argv);
	return ret;
}


static int add_disk_param(struct CParam *param, struct vzctl_disk_param **disk,
		int id, const char *val)
{
	if (*disk == NULL)
		*disk = calloc(1, sizeof(struct vzctl_disk_param));

	if (id == PARAM_DEVICE_ADD) {
		if (strcmp(val, "hdd")) {
			fprintf(stderr, "Incorrect device type is specified: %s\n", val);
			return VZ_INVALID_PARAMETER_VALUE;
		}
		if (gparam->disk_op != DEVICE_ACTION_UNSPEC)
			return 0;
		gparam->disk_op = DEVICE_ACTION_ADD;
	} else if (id == PARAM_DEVICE_DEL) {
		if (gparam->disk_op != DEVICE_ACTION_UNSPEC)
			return 0;
		if (vzctl_get_normalized_guid(val, (*disk)->uuid, sizeof(*disk)->uuid)) {
			fprintf(stderr, "Incorrect device uuid specified: %s\n", val);
			return VZ_INVALID_PARAMETER_VALUE;
		}

		gparam->disk_op = DEVICE_ACTION_DEL;
	} else if (id == PARAM_DEVICE_SET) {
		if (gparam->disk_op != DEVICE_ACTION_UNSPEC)
			return 0;
		if (vzctl_get_normalized_guid(val, (*disk)->uuid, sizeof(*disk)->uuid)) {
			fprintf(stderr, "Incorrect device uuid specified: %s\n", val);
			return VZ_INVALID_PARAMETER_VALUE;
		}
		gparam->disk_op = DEVICE_ACTION_SET;
	} else if (id == PARAM_IMAGE) {
		(*disk)->path = strdup(val);
		(*disk)->use_device = 0;
	} else if (id == PARAM_DEVICE) {
		(*disk)->path = strdup(val);
		(*disk)->use_device = 1;
	} else if (id == PARAM_SIZE) {
		unsigned long size[2];

		if (ParseTwoLongs(val, size, TWOULONG_K)) {
			fprintf(stderr, "Incorrect size is specified: %s\n", val);
			return VZ_INVALID_PARAMETER_VALUE;
		}
		(*disk)->size = size[1];
	} else if (id == PARAM_DEVICE_UUID) {
		if (vzctl_get_normalized_guid(val, (*disk)->uuid, sizeof(*disk)->uuid)) {
			fprintf(stderr, "Incorrect device uuid specified: %s\n", val);
			return VZ_INVALID_PARAMETER_VALUE;
		}
	} else if (id == PARAM_ENABLE) {
		(*disk)->enabled = VZCTL_PARAM_ON;
	} else if (id == PARAM_DISABLE) {
		(*disk)->enabled = VZCTL_PARAM_OFF;
	} else if (id == PARAM_DISK_MNT) {
		xstrdup(&(*disk)->mnt, val);
	} else if (id == PARAM_ENC_KEYID) {
		xstrdup(&(*disk)->enc_keyid, val);
	} else if (id == PARAM_DISK_OFFLINE) {
		(*disk)->offline_resize = 1;
	} else if (id == PARAM_DISK_STORAGE_URL) {
		(*disk)->storage_url = strdup(val);
	} else if (id == PARAM_DISK_DETACH)
		gparam->disk_op = DEVICE_ACTION_DETACH;


	return 0;
}

static int validate_disk_param(struct CParam *param, struct vzctl_disk_param *disk)
{
	if (disk == NULL) {
		if (param->quota_block == NULL)
			return 0;
		/* set VEID --diskspace */
		disk = calloc(1, sizeof(struct vzctl_disk_param));
	}

	if (gparam->disk_op == DEVICE_ACTION_UNSPEC) {
		/* set VEID --size N [--offline] */
		gparam->disk_op = DEVICE_ACTION_SET;
		strcpy(disk->uuid, DISK_ROOT_UUID);
		if (param->quota_block != NULL)
			disk->size = param->quota_block[1];

		if (disk->size == 0) {
			fprintf(stderr, "Incorrect size is specified: %lu\n",
					disk->size);
			return -1;
		}
	} else if (!param->save && !disk->use_device) {
		fprintf(stderr, "The --save option have to be used with --device-*\n");
		return -1;
	}

	/* --device-set UUID --blocksize N */
	if (gparam->disk_op == DEVICE_ACTION_SET &&
			disk->size == 0 && param->quota_block != NULL)
	{
		if (strcmp(disk->uuid, DISK_ROOT_UUID)) {
			disk->size = param->quota_block[1];
			free(param->quota_block);
			param->quota_block = NULL;
		}
		disk->size = param->quota_block[1];
	}

	param->disk_param = disk;

	return 0;
}

static int find_arg(char **arg, const char *str)
{
	char **p;

	for (p = arg; *p != NULL; p++)
		if (!strcmp(*p, str))
			return 1;
	return 0;
}

static void convert_short_opt_to_val(int *c)
{
	size_t i, j;
	size_t altlen = sizeof(option_alternatives) / sizeof(option_alternatives[0]);
	size_t optionslen = sizeof(set_options) / sizeof(set_options[0]);

	for (i = 0; i < altlen; ++i)
		if (option_alternatives[i].short_name == *c)
			for (j = 0; j < optionslen; ++j)
				if (!strcmp(option_alternatives[i].long_name, set_options[j].name)) {
					*c = set_options[j].val;
					return;
				}
}

int ParseSetOptions(ctid_t ctid, struct CParam *param, int argc, char **argv)
{
	int i, c, err, ret = 0, bindmount = 0;
	struct vzctl_disk_param *disk = NULL;
	int veth = find_arg(argv, "--ifname") || find_arg(argv, "--netif_add");

	memset(&tmpparam, 0, sizeof(tmpparam));
	while (1)
	{
		int option_index = -1;

		c = getopt_long (argc, argv, PARAM_LINE, set_options,
				&option_index);
		if (c == -1)
			break;
		if (c == '?')
			return VZ_INVALID_PARAMETER_VALUE;

		switch (c) {
		case PARAM_DEVICE_ADD:
		case PARAM_DEVICE_DEL:
		case PARAM_DEVICE_SET:
		case PARAM_IMAGE:
		case PARAM_DEVICE:
		case PARAM_SIZE:
		case PARAM_DEVICE_UUID:
		case PARAM_ENABLE:
		case PARAM_DISABLE:
		case PARAM_DISK_MNT:
		case PARAM_ENC_KEYID:
		case PARAM_DISK_OFFLINE:
		case PARAM_DISK_STORAGE_URL:
		case PARAM_DISK_DETACH:
			ret = add_disk_param(&tmpparam, &disk, c, optarg);
			if (ret)
				return ret;
			continue;
		case PARAM_ENC_REENCRYPT:
			gparam->enc_flags |= VZCTL_ENC_REENCRYPT;
			continue;
		case PARAM_ENC_WIPE:
			gparam->enc_flags |= VZCTL_ENC_WIPE;
			continue;
		case PARAM_RECREATE:
			gparam->recreate = 1;
			continue;
		case PARAM_IP_ADD:
			c = veth ? VZCTL_PARAM_NETIF_IPADD :
					VZCTL_PARAM_IP_ADDRESS;
			break;
		case PARAM_IP_DEL:
			c = veth ? VZCTL_PARAM_NETIF_IPDEL:
					VZCTL_PARAM_IPDEL;
			break;
		case VZCTL_PARAM_NETIF_ADD:
			/* use ifname from --netif_add <ifname> */
			if (!find_arg(argv, "--ifname")) {
				ret = vzctl_add_env_param_by_id(ctid,
						VZCTL_PARAM_NETIF_IFNAME, optarg);
				if (ret)
					return vzctl_err(ret, 0, "Bad parameter for"
							" --%s: %s",
							set_options[option_index].name, optarg);
			}
			break;
		case VZCTL_PARAM_BINDMOUNT:
		case VZCTL_PARAM_BINDMOUNT_DEL:
			bindmount = 1;
			break;
		case PARAM_CAP:
			fprintf(stderr, "Warning: The --capability option"
					" is deprecated\n");
			continue;
		}

		if (isalpha(c))
			convert_short_opt_to_val(&c);

		if (c >= VZCTL_PARAM_KMEMSIZE) {
			if(c == VZCTL_PARAM_SWAP || c == VZCTL_PARAM_SWAPPAGES)
				fprintf(stderr, "WARNING: Use of swap significantly slows down both the container and the node.\n");

			ret = vzctl_add_env_param_by_id(ctid, c, optarg);
			if (ret) {
				if (option_index < 0)
					return vzctl_err(ret, 0, "Bad parameter for"
							" -%c: %s", c, optarg);
				else
					return vzctl_err(ret, 0, "Bad parameter for"
							" --%s: %s", set_options[option_index].name, optarg);
			}
			continue;
		}

		if ((i = FindOption(c)) < 0)
		{
			ret = VZ_INVALID_PARAMETER_SYNTAX;
			break;
		}
		err = SetParam(i, optarg, 0, gparam->version, 0);
		if (err == ERR_DUP)
			return VZ_INVALID_PARAMETER_SYNTAX;
		else if (err == ERR_INVAL || err == ERR_INVAL_SKIP)
		{
			if (option_index < 0)
			{
				logger(-1, 0, "Bad parameter for"
						" -%c: %s", c, optarg);
				return VZ_INVALID_PARAMETER_VALUE;
			}
			else
			{
				logger(-1, 0, "Bad parameter for"
			" --%s: %s", set_options[option_index].name, optarg);
				return VZ_INVALID_PARAMETER_VALUE;
			}
		}
		else if (err == ERR_UNK && gparam->version)
		{

			if (option_index < 0)
			{
				logger(-1, 0, "Invalid option"
					" -%c for version %d",
					c, gparam->version);
				return VZ_INVALID_PARAMETER_SYNTAX;
			}
			else
			{
				logger(-1, 0, "Invalid option"
					" --%s for version %d",
					set_options[option_index].name,
					gparam->version);
				return VZ_INVALID_PARAMETER_SYNTAX;
			}
		}
	}
	memcpy(param, &tmpparam, sizeof(tmpparam));

	if (validate_disk_param(param, disk))
		return VZ_INVALID_PARAMETER_SYNTAX;

	if (!param->save && bindmount) {
		fprintf(stderr, "The --bindmount-* options must be used together with the --save option.\n");
		return VZ_INVALID_PARAMETER_SYNTAX;
	}

	ret = check_argv_tail(argc, argv);
	return ret;
}

int ParseUnsetOptions(ctid_t ctid, struct CParam *param, int argc, char **argv)
{
	int i, c, ret = 0;
	struct option unset_options[sizeof(set_options)];
	struct option *op;

	memcpy(unset_options, set_options, sizeof(set_options));
	for (op = unset_options; op->name != NULL; op++)
		op->has_arg = no_argument;
	memset(&tmpparam, 0, sizeof(tmpparam));
	while (1)
	{
		int option_index = -1;

		c = getopt_long (argc, argv, "", unset_options,
				&option_index);
		if (c == -1)
			break;
		if (c == '?')
			return VZ_INVALID_PARAMETER_VALUE;
		if (c >= VZCTL_PARAM_KMEMSIZE) {
			if (vzctl_del_param_by_id(ctid, c)) {
				fprintf(stderr, "Inknown parameter %d\n", c);
				return VZ_INVALID_PARAMETER_SYNTAX;
			}
		} else {
			if ((i = FindOption(c)) < 0)
				return VZ_INVALID_PARAMETER_SYNTAX;
			SetParam(i, "", 0, gparam->version, 1);
		}
	}
	memcpy(param, &tmpparam, sizeof(tmpparam));
	ret = check_argv_tail(argc, argv);
	return ret;
}

static int print_script(char *root, struct CList *list, int desc)
{
	FILE *fp;
	char *name, *p;
	struct CList *l;
	char buf[4096];
	int i;

	for (l = list; l != NULL; l = l->next) {
		if ((name = strrchr(l->str, '/')) == NULL)
			name = l->str;
		if ((p = strrchr(l->str, '.')) != NULL)
			*p = 0;
		snprintf(buf, sizeof(buf), "%s" CUSTOM_SCRIPT_DIR "/%s.desc",
			root,l->str);
		if (p != NULL)
			*p = '.';
		if (desc == YES) {
			if ((fp = fopen(buf, "r")) != NULL) {
				i = 0;
				buf[0] = 0;
				fprintf(stdout, "%s:", name);
				while ((fgets(buf, sizeof(buf), fp)) != NULL) {
					if (i++ == 0)
						fprintf(stdout, "%s", buf);
					else
						fprintf(stdout, "%s:%s",
							name, buf);
				}
				i = strlen(buf);
				if (i == 0 || buf[i - 1] != '\n')
					fprintf(stdout, "\n");
				fclose(fp);
			} else {
				fprintf(stdout, "%s:\n", name);
			}
		} else {
			fprintf(stdout, "%s\n", name);
		}
	}
	return 0;
}

int get_reinstall_scripts(char *root, struct CList **list)
{
	char buf[STR_SIZE*2+1];
	char dir[STR_SIZE];
	struct stat st;
	struct dirent *ep;
	DIR *dp;
	int changed;
	char *tmp;
	struct CList *l;

	snprintf(dir, sizeof(dir), "%s" CUSTOM_SCRIPT_DIR, root);
	if (!(dp = opendir(dir)))
		return 0;
	while ((ep = readdir(dp))) {
		const char *p = strrchr(ep->d_name, '.');
		if (p == NULL || strcmp(p, ".sh"))
			continue;
		snprintf(buf, sizeof(buf), "%s/%s", dir, ep->d_name);
		stat(buf, &st);
		if (S_ISREG(st.st_mode) && st.st_mode & S_IXUSR)
			*list = ListAddElem(*list, ep->d_name, NULL, NULL);
	}
	do {
		changed = 0;
		for (l = *list; l != NULL && l->next != NULL; l = l->next) {
			if (strcmp(l->str, l->next->str) <= 0)
				continue;
			tmp = l->str;
			l->str = l->next->str;
			l->next->str = tmp;
			changed++;
		}
	} while (changed);
	closedir(dp);
	return 0;
}

static int reinstall_list(ctid_t ctid, struct CParam *param)
{
	int mounted, ret;
	struct CList *ve0_list = NULL, *ve_list = NULL;

	if (check_var(param->ve_root, "VE_ROOT is not set"))
		return VZ_VE_ROOT_NOTSET;
	get_reinstall_scripts("", &ve0_list);
	if (!(mounted = env_is_mounted(ctid))) {
		vzctl_set_log_quiet(1);
		if ((ret = vzctl_env_mount(ctid, 0)))
			return ret;
	}
	get_reinstall_scripts(param->ve_root, &ve_list);
	print_script("", ve0_list, param->reinstall_desc);
	print_script(param->ve_root, ve_list, param->reinstall_desc);
	if (!mounted)
		vzctl_env_umount(ctid);
	ListFree(ve0_list);
	ListFree(ve_list);
	return 0;
}


int Show(ctid_t ctid)
{
	vzctl_env_status_t status = {};

	vzctl2_get_env_status(ctid, &status, ENV_STATUS_ALL);

	printf("VEID %s %s %s", ctid,
		status.mask & ENV_STATUS_EXISTS ? "exist" : "deleted",
		status.mask & ENV_STATUS_MOUNTED ? "mounted" : "unmounted");
	
	if (status.mask & ENV_STATUS_RUNNING)
		printf(" %s", status.mask & ENV_STATUS_CPT_SUSPENDED ?
				"paused" : "running");
	else
		printf(" %s", status.mask & ENV_STATUS_SUSPENDED ? 
				"suspended" :"down");
	printf("\n");
	return 0;
}

static void cleanup_stail_registration(void)
{
	DIR *dir;
	struct dirent *ent;
	char *p;
	int ret, len;
	char conf[PATH_MAX];
	char path[PATH_MAX];

	if ((dir = opendir(ENV_CONF_DIR)) == NULL)
		return;
	while ((ent = readdir(dir)) != NULL) {
		if ((p = strrchr(ent->d_name, '.')) == NULL || strcmp(p, ".conf"))
			continue;

		snprintf(conf, sizeof(conf), ENV_CONF_DIR "%s", ent->d_name);
		len = readlink(conf, path, sizeof(path) - 1);
		if (len == -1)
			continue;
		path[len] = 0;

		if ((p = strrchr(path, '/')) == NULL)
			continue;

		if (strcmp(p + 1, VZCTL_VE_CONF))
			continue;
		*p = '\0';

		ret = vzctl2_check_owner(path);
		if (ret == VZ_VE_MANAGE_DISABLED) {
			logger(-1, 0, "Cleanup stail registration %s", conf);
			if (unlink(conf))
				logger(-1, errno, "Failed to unlink the %s", conf);
		}
	}
	closedir(dir);
}

static int get_flags(struct CParam *param)
{
	return (gparam->wait ? VZCTL_WAIT : 0) |
		(gparam->skip_ve_setup ? VZCTL_SKIP_SETUP : 0) |
		(gparam->skip_fsck ? VZCTL_SKIP_FSCK : 0) |
		(gparam->skip_arpdetect ? VZCTL_SKIP_ARPDETECT : 0) |
		(gparam->ignore_ha_cluster == YES ? VZCTL_SKIP_HA_REG : 0) |
		(gparam->flags);
}

void Version(void);
void Usage(void);
int main(int argc, char **argv, char *envp[])
{
	int action = 0;
	char buf[PATH_MAX];
	struct CParam *param;
	int lckfd = -1;
	int ret = 0;
	int skiplock = 0;
	int verbose = 0, skipowner = 0;
	int quiet = 0;
	char *status = NULL;
	struct sigaction act;
	int console_tty = 2;
	int start_console = 0;
	char **argv_orig = argv;
	struct stat st;
	const char *ve_private = NULL;
	int reg_flags = 0;
	struct vzctl_env_handle *h = NULL;
	ctid_t ctid = {};
	int flags = 0;

	param = xcalloc(1, sizeof(struct CParam));
	gparam = xcalloc(1, sizeof(struct CParam));

	sigemptyset(&act.sa_mask);
	act.sa_handler = SIG_DFL;
	act.sa_flags = 0;
	sigaction(SIGTTOU, &act, NULL);
	sigaction(SIGTTIN, &act, NULL);
	act.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &act, NULL);
	reset_std();

	while (argc > 1)
	{
		if (!STRNCOMP(argv[1], "--verbose"))
			verbose = 5;
		else if (!STRNCOMP(argv[1], "--quiet"))
			quiet = 1;
		else if (!STRNCOMP(argv[1], "--help"))
		{
			Usage();
			exit(0);
		}
		else if (!STRNCOMP(argv[1], "--version"))
		{
			Version();
			exit(0);
		}
		else if (!strcmp(argv[1], "--skiplock"))
			skiplock = YES;
		else if (!strcmp(argv[1], "--skipowner"))
			skipowner = YES;
		else if (!strcmp(argv[1], "--ignore-ha-cluster"))
			gparam->ignore_ha_cluster = YES;
		else if (!strcmp(argv[1], "--configure-timeout")) {
			int val;

			if (argv[2] == NULL || parse_int(argv[2], &val)) {
				fprintf(stderr, "An incorrect value '%s' for %s is specified\n",
						argv[2], argv[1]);
				exit(1);
			}
			/* TODO:
			set_configure_timeout(val);
			*/
			argc--; argv++;
		} else
			break;
		argc--; argv++;
	}

	if (argc <= 1)
	{
		Usage();
		exit(VZ_INVALID_PARAMETER_SYNTAX);
	}
	if (!strcmp(argv[1], "set"))
	{
		action = ACTION_SET;
		status = ST_SET;
	}
	else if (!strcmp(argv[1], "setrate"))
	{
		action = ACTION_SET_RATE;
		status = ST_SET;
	}
	else if (!strcmp(argv[1], "unset"))
	{
		action = ACTION_UNSET;
	}
	else if (!strcmp(argv[1], "create"))
	{
		action = ACTION_CREATE;
		status = ST_CREATE;
	}
	else if (!strcmp(argv[1], "start"))
	{
		action = ACTION_START;
		status = ST_START;
	}
	else if (!strcmp(argv[1], "pause"))
	{
		action = ACTION_PAUSE;
	}
	else if (!strcmp(argv[1], "stop"))
	{
		action = ACTION_STOP;
		status = ST_STOP;
	}
	else if (!strcmp(argv[1], "restart"))
	{
		action = ACTION_RESTART;
		status = ST_RESTART;
	}
	else if (!strcmp(argv[1], "destroy") || !strcmp(argv[1], "delete"))
	{
		action = ACTION_DESTROY;
		status = ST_DESTROY;
	}
	else if (!strcmp(argv[1], "mount"))
	{
		action = ACTION_MOUNT;
		status = ST_MOUNT;
	}
	else if (!strcmp(argv[1], "umount"))
	{
		action = ACTION_UMOUNT;
		status = ST_UMOUNT;
	}
	else if (!strcmp(argv[1], "exec3"))
	{
		action = ACTION_EXEC3;
	}
	else if (!strcmp(argv[1], "exec2"))
	{
		action = ACTION_EXEC2;
	}
	else if (!strcmp(argv[1], "exec"))
	{
		action = ACTION_EXEC;
	}
	else if (!strcmp(argv[1], "runscript"))
	{
		action = ACTION_RUNSCRIPT;
	}
	else if (!strcmp(argv[1], "execaction"))
	{
		action = ACTION_EXECACTION;
	}
	else if (!strcmp(argv[1], "enter"))
	{
		action = ACTION_ENTER;
	}
	else if (!strcmp(argv[1], "restore"))
	{
		action = ACTION_RESTORE;
		status = ST_RESTORE;
	}
	else if (!strcmp(argv[1], "resume"))
	{
		action = ACTION_RESTORE;
		status = ST_RESUME;
	}
	else if (!strcmp(argv[1], "chkpnt"))
	{
		action = ACTION_CHKPNT;
	}
	else if (!strcmp(argv[1], "suspend"))
	{
		action = ACTION_SUSPEND;
		status = ST_SUSPEND;
	}
	else if (!strcmp(argv[1], "snapshot"))
	{
		action = ACTION_SNAPSHOT;
		status = ST_SNAPSHOT;
	}
	else if (!strcmp(argv[1], "tsnapshot"))
	{
		action = ACTION_TSNAPSHOT;
		status = ST_SNAPSHOT;
	}
	else if (!strcmp(argv[1], "snapshot-switch"))
	{
		action = ACTION_SNAPSHOT_SWITCH;
		status = ST_SNAPSHOT_SWITCH;
	}
	else if (!strcmp(argv[1], "snapshot-delete"))
	{
		action = ACTION_SNAPSHOT_DELETE;
		status = ST_SNAPSHOT_DELETE;
	}
	else if (!strcmp(argv[1], "tsnapshot-delete"))
	{
		action = ACTION_TSNAPSHOT_DELETE;
		status = ST_SNAPSHOT_DELETE;
	}
	else if (!strcmp(argv[1], "snapshot-list"))
	{
		quiet = 1;
		action = ACTION_SNAPSHOT_LIST;
	}
	else if (!strcmp(argv[1], "snapshot-mount"))
	{
		action = ACTION_SNAPSHOT_MOUNT;
	}
	else if (!strcmp(argv[1], "snapshot-umount"))
	{
		action = ACTION_SNAPSHOT_UMOUNT;
	}
	else if (!strcmp(argv[1], "status"))
	{
		action = ACTION_STATUS;
	}
	else if (!strcmp(argv[1], "reinstall"))
	{
		action = ACTION_REINSTALL;
		status = ST_REINSTALL;
	}
	else if (!strcmp(argv[1], "monitor"))
	{
		action = ACTION_MONITOR;
	}
	else if (!strcmp(argv[1], "register"))
	{
		if (argc < 3) {
			fprintf(stderr, "Usage: vzctl register <path>\n");
			exit(VZ_INVALID_PARAMETER_SYNTAX);
		}

		ve_private = argv[2];
		argv++; argc--;

		action = ACTION_REGISTER;
		if (argc == 2)
			goto skip_eid;
	}
	else if (!strcmp(argv[1], "unregister"))
	{
		action = ACTION_UNREGISTER;
	}
	else if (!strcmp(argv[1], "convert"))
	{
		action = ACTION_CONVERT;
	}
	else if (!strcmp(argv[1], "console"))
	{
		action = ACTION_CONSOLE;
	}
	else if (!strcmp(argv[1], "compact"))
	{
		action = ACTION_COMPACT;
	}
	else if (!strcmp(argv[1], "cleanup-stail-registration"))
	{
		cleanup_stail_registration();
		exit(0);
	}
	else if (!strcmp(argv[1], "--help"))
	{
		Usage();
		exit(0);
	}
	else
	{
		if (!strcmp(argv[1], "quotaon") ||
			!strcmp(argv[1], "quotaoff") ||
			!strcmp(argv[1], "quotainit"))
		{
			fprintf(stderr, "Warning: The '%s' action is deprecated\n", argv[1]);
			ret = 0;
		} else {
			fprintf(stderr, "Bad command: %s\n", argv[1]);
			ret = VZ_INVALID_PARAMETER_SYNTAX;
		}
		goto END;
	}

	if (argc < 3) {
		fprintf(stderr, "ctid is not given\n");
		ret = VZ_INVALID_PARAMETER_VALUE;
		goto END;
	}

	ret = VZ_INVALID_PARAMETER_VALUE;
	if (action == ACTION_CREATE || action == ACTION_REGISTER) {
		if (vzctl2_parse_ctid(argv[2], ctid)) {
			fprintf(stderr, "Invalid ctid is specified: %s\n", argv[2]);
			goto END;
		}
	} else {
		char name[STR_SIZE];

		if (vzctl_convertstr(argv[2], name, sizeof(name)))
			goto END;

		if (vzctl2_get_envid_by_name(name, ctid) &&
				vzctl2_parse_ctid(argv[2], ctid))
		{
			fprintf(stderr, "Invalid ctid is specified: %s\n", argv[2]);
			goto END;
		}
	}

skip_eid:
	argc -= 2;
	argv += 2;


	/* do not any checkings for monitoring and console */
	if (action == ACTION_MONITOR)
		return monitoring(ctid);
	get_pagesize();
	vzctl_init_log(ctid, quiet, "vzctl");
	if (verbose)
		vzctl2_set_log_verbose(5);

	if ((ret = vzctl2_lib_init()))
		goto END;

	int f = VZCTL_FLAG_DONT_USE_WRAP |
		(getenv("VZCTL_FLAG_DONT_SEND_EVT") ? VZCTL_FLAG_DONT_SEND_EVT : 0);
	vzctl2_set_flags(f);

	gparam->skiplock = skiplock;
	switch (action)
	{
	case ACTION_REGISTER:
		if ((ret = ParseRegisterOptions(argc, argv, &reg_flags, param)))
			goto END;
		reg_flags |= (skipowner == YES ? VZ_REG_FORCE : 0);
		if (gparam->ignore_ha_cluster == YES)
			reg_flags |= VZ_REG_SKIP_HA_CLUSTER;
		if (!(reg_flags & VZ_REG_START))
			skiplock = YES;
		break;
	case ACTION_SET		:
	case ACTION_SET_RATE:
	{
		if ((ret = ParseSetOptions(ctid, param, argc, argv)))
			goto END;
		break;
	}
	case ACTION_UNSET		:
	{
		if ((ret = ParseUnsetOptions(ctid, param, argc, argv)))
			goto END;
		break;
	}
	case ACTION_CREATE	:
	{
		if ((ret = ParseCreateOptions(ctid, param, argc, argv)))
			goto END;
		break;
	}
	case ACTION_STOP	:
		if ((ret = ParseStopOptions(param, argc, argv)))
			 goto END;
		break;
	case ACTION_START	:
	case ACTION_RESTART	:
		if ((ret = ParseStartOptions(param, argc, argv)))
			 goto END;
		break;
	case ACTION_PAUSE	:
		break;
	case ACTION_SUSPEND	:
	case ACTION_CHKPNT	:
		if ((ret = ParseMigrationOptions(param, chkpnt_options,
				argc, argv)))
			goto END;
		if (!gparam->cpt_cmd)
			gparam->cpt_cmd = VZCTL_CMD_CHKPNT;
		break;
	case ACTION_RESTORE	:
		if ((ret = ParseMigrationOptions(param, restore_options,
				argc, argv)))
			goto END;
		if (!gparam->cpt_cmd)
			gparam->cpt_cmd = VZCTL_CMD_RESTORE;
		break;
	case ACTION_SNAPSHOT:
	case ACTION_TSNAPSHOT:
		if ((ret = ParseSnapshotOptions(param, snapshot_options, argc, argv)))
			goto END;
		break;
	case ACTION_SNAPSHOT_SWITCH:
		if ((ret = ParseSnapshotOptions(param, snapshot_switch_options, argc, argv)))
			goto END;
		if (param->snapshot_guid == NULL) {
			fprintf(stderr, "Snapshot uuid is not specified.\n");
			ret = VZ_INVALID_PARAMETER_SYNTAX;
			goto END;
		}
		break;
	case ACTION_SNAPSHOT_DELETE:
	case ACTION_TSNAPSHOT_DELETE:
		if ((ret = ParseSnapshotOptions(param, snapshot_delete_options, argc, argv)))
			goto END;
		if (param->snapshot_guid == NULL) {
			fprintf(stderr, "Snapshot uuid is not specified.\n");
			ret = VZ_INVALID_PARAMETER_SYNTAX;
			goto END;
		}
		break;
	case ACTION_SNAPSHOT_MOUNT:
		if ((ret = ParseSnapshotOptions(param, snapshot_mount_options, argc, argv)))
			goto END;
		if (param->snapshot_guid == NULL) {
			fprintf(stderr, "Snapshot uuid is not specified.\n");
			ret = VZ_INVALID_PARAMETER_SYNTAX;
			goto END;
		}
		if (param->target_dir == NULL) {
			fprintf(stderr, "Mount direcory is not specified.\n");
			ret = VZ_INVALID_PARAMETER_SYNTAX;
			goto END;
		}
		break;
	case ACTION_SNAPSHOT_UMOUNT:
		if ((ret = ParseSnapshotOptions(param, snapshot_umount_options, argc, argv)))
			goto END;
		if (param->snapshot_guid == NULL) {
			fprintf(stderr, "Snapshot uuid is not specified.\n");
			ret = VZ_INVALID_PARAMETER_SYNTAX;
			goto END;
		}
		break;
	case ACTION_DESTROY	:
		if ((ret = ParseDelOptions(param, argc, argv)))
			 goto END;
		break;
	case ACTION_MOUNT	:
		if ((ret = ParseMountOptions(param, argc, argv)))
			goto END;
		break;
	case ACTION_REINSTALL	:
		if ((ret = ParseReinstallOptions(param, argc, argv)))
			goto END;
		if (param->reinstall_list)
			// disable logging to stdout
			vzctl_set_log_verbose(-1);

		break;
	case ACTION_EXEC :
	case ACTION_EXEC2 :
	case ACTION_EXEC3 :
	case ACTION_RUNSCRIPT:
	case ACTION_EXECACTION:
		if (argc < 2)
		{
			fprintf(stderr, "No command to execute by vzctl exec\n");
			ret = VZ_INVALID_PARAMETER_SYNTAX;
			goto END;
		}
		// disable logging to stdout
		vzctl_set_log_verbose(-1);
		break;
	case ACTION_CONVERT:
		if ((ret = ParseConvertOptions(param, argc, argv)))
			goto END;
		break;
	case ACTION_STATUS:
		// disable logging to stdout
		vzctl_set_log_verbose(-1);
		break;
	case ACTION_CONSOLE:
		if ((ret = ParseConsoleOptions(argc, argv,
				&start_console, &console_tty)))
			goto END;
		break;
	case ACTION_SNAPSHOT_LIST:
		break;
	case ACTION_COMPACT:
		if ((ret = ParseCompactOptions(param, argc, argv)))
			 goto END;
		break;
	default :
	{
		if ((argc - 1) > 0)
		{
			fprintf (stderr, "Invalid options: ");
			while (--argc) fprintf(stderr, "%s ", *(++argv));
			fprintf (stderr, "\n");
			ret = VZ_INVALID_PARAMETER_SYNTAX;
			goto END;
		}
	}
	}
	/* Read Container config file */
	if (action == ACTION_CONVERT && param->config != NULL)
		snprintf(buf, sizeof(buf), "%s", param->config);
	else if (action == ACTION_REGISTER) {
		buf[0] = '\0';
	} else {
		vzctl2_get_env_conf_path(ctid, buf, sizeof(buf));
	}

	if (!stat_file(buf)) {
		if (action == ACTION_STOP && gparam->fastkill &&
				lstat(buf, &st) == 0) {
			action = ACTION_STOP_FORCE;
			flags |= VZCTL_CONF_SKIP_PARSE;
		}

		if (action != ACTION_CREATE &&
			action != ACTION_STOP &&
			action != ACTION_STOP_FORCE &&
			action != ACTION_SET &&
			action != ACTION_SET_RATE &&
			action != ACTION_STATUS &&
			action != ACTION_REGISTER)
		{
			logger(-1, 0, "Container configuration file does not"
					" exist");
			ret = VZ_NOVE_CONFIG;
			goto END;
		}
		flags |= VZCTL_CONF_SKIP_NON_EXISTS;
	}

	h = vzctl_env_open(ctid, buf, flags, &ret);
	if (ret)
		goto END;

	if (h && !(skipowner ||
			action == ACTION_CREATE ||
			action == ACTION_STATUS ||
			action == ACTION_STOP_FORCE ||
			(action == ACTION_STOP && param->fastkill) ||
			action == ACTION_UNREGISTER ||
			action == ACTION_REGISTER))
	{
		const char *ve_private = NULL;

		vzctl2_env_get_ve_private_path(vzctl2_get_env_param(h), &ve_private);
		if ((ret = vzctl2_check_owner(ve_private)))
			goto END;
	}

	if (action == ACTION_START || action == ACTION_STOP ||
		action == ACTION_SET || action == ACTION_CREATE ||
		action == ACTION_MOUNT || action == ACTION_UMOUNT ||
		action == ACTION_DESTROY || action == ACTION_QUOTA ||
		action == ACTION_REINSTALL ||
		action == ACTION_RESTART || action == ACTION_RUNSCRIPT ||
		action == ACTION_RESTORE || action == ACTION_CHKPNT ||
		action == ACTION_EXECACTION || action == ACTION_CONVERT ||
		action == ACTION_UNSET || action == ACTION_SUSPEND ||
		action == ACTION_SNAPSHOT || action == ACTION_SNAPSHOT_SWITCH ||
		action == ACTION_SNAPSHOT_DELETE || action == ACTION_SNAPSHOT_MOUNT ||
		action == ACTION_SNAPSHOT_UMOUNT || action == ACTION_REGISTER ||
		action == ACTION_TSNAPSHOT || action == ACTION_TSNAPSHOT_DELETE ||
		action == ACTION_PAUSE || action == ACTION_COMPACT)
	{
		lckfd = 0;
		if (skiplock != YES) {
			if (action == ACTION_DESTROY)
				/* Skip VE_PRIVATE locking */
				lckfd = vzctl2_env_lock_prvt(ctid, NULL, status);
			else
				lckfd = vzctl2_env_lock(h, status);
		}
		if (lckfd == -2)
		{
			logger(-1, 0, "Cannot lock the Container");
			ret = VZ_LOCKED;
			goto END;
		}
		else if (lckfd == -1)
		{
			ret = VZ_SYSTEM_ERROR;
			goto END;
		}
	}

	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = cleanup_callback;
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGHUP, &act, NULL);

	switch (action)
	{
		case ACTION_REGISTER:
		{
			struct vzctl_reg_param reg = {
				.uuid = param->guid,
				.name = param->name,
			};

			if (!EMPTY_CTID(ctid))
				snprintf(reg.ctid, sizeof(reg.ctid), "%s", ctid);

			ret = vzctl2_env_register(ve_private, &reg, reg_flags);
			if (ret)
				break;
			/* ignore start error */
			if (reg_flags & VZ_REG_START) {
				vzctl_env_close();
				gparam->skip_ve_setup = 0;
				gparam->skip_fsck = 0;
				vzctl_env_start(ctid, NULL, get_flags(gparam));
			}
			break;
		}
		case ACTION_START	:
		{
			vzctl_env_status_t status = {};

			ret = vzctl2_get_env_status(ctid, &status, ENV_STATUS_SUSPENDED);
			if (ret)
				break;

			if (status.mask & ENV_STATUS_SUSPENDED) {
				ret = vzctl_env_restore(ctid, VZCTL_CMD_RESTORE,
						gparam->dumpfile,
						gparam->cptcontext,
						gparam->cpu_flags,
						gparam->cpt_flags,
						get_flags(gparam));
				if (ret == 0)
					break;
			}
			ret = vzctl_env_start(ctid, gparam->cidata_fname, get_flags(gparam));
			break;
		}
		case ACTION_STOP_FORCE	:
		{
			ret = vzctl_env_stop(ctid, M_KILL_FORCE, gparam->stop_flags);
			break;
		}
		case ACTION_STOP	:
		{
			ret = vzctl_env_stop(ctid, gparam->fastkill ? M_KILL : M_HALT,
					gparam->stop_flags);
			break;
		}
		case ACTION_RESTART	:
		{
			ret = vzctl_env_restart(ctid, gparam->wait, gparam->skip_ve_setup);
			break;
		}
		case ACTION_PAUSE	:
			ret = vzctl_env_pause(ctid);
			break;
		case ACTION_CREATE	:
		{
			ret = vzctl_env_create(ctid,
					gparam->guid,
					gparam->config,
					gparam->ostmpl,
					gparam->ve_private,
					gparam->ve_root,
					gparam->name,
					gparam->enc_keyid,
					gparam->no_hdd,
					gparam->ve_layout);
			break;
		}
		case ACTION_SET_RATE:
		{
			ret = vzctl_env_set_rate(ctid);
			break;
		}
		case ACTION_SET		:
		{
			struct CParam *p = param;
			if (param->applyconfig != NULL) {
				if ((ret = vzctl_env_open_apply_conf(ctid, param->applyconfig)))
					break;
			}
			if (!param->save) {
				ret = VZ_INVALID_PARAMETER_SYNTAX;
				if (param->setmode == VZCTL_SET_RESTART) {
					logger(-1, 0, "It is not allowed to use"
							" restart mode without --save");
					break;
				}
				if (param->name != NULL) {
					logger(-1, 0, "Unable to use"
							" the --name option without --save");
					ret = VZ_SET_NAME_ERROR;
					break;
				}
				if (param->quota_block != NULL &&
						gparam->ve_layout == VZCTL_LAYOUT_5) {
					logger(-1, 0, "It is not allowed to use"
							" the --diskspace option without --save");
					break;
				}
			}
#if 0
			if ((ignore_ha_cluster != YES) && ctid) {
				struct ha_params cmdline_params = {
					.ha_enable = param->ha_enable,
					.ha_prio = param->ha_prio,
				};
				struct ha_params config_params = {
					.ha_enable = gparam->ha_enable,
					.ha_prio = gparam->ha_prio,
				};
				ret = handle_set_cmd_on_ha_cluster(ctid, gparam->ve_private,
							   &cmdline_params, &config_params,
							   param->save);
				if (ret)
					break;
			}
#endif
			if (*ctid == '\0')
				ret = Set_ve0(p);
			else
				ret = Set(ctid, p);
			if (p->save)
			{
				if (vzctl_env_save(ctid) == 0)
					logger(0, 0, "Saved parameters for Container %s", ctid);
			}
			else
			{
				if (p->userpw == NULL && !p->resetub)
				{
					fprintf(stdout, "WARNING:"
						" Changes are not saved and will be dropped on the next start."
						" Suspend or migrate operations may fail."
						" Use --save to save the settings in the configuration file.\n");
				}
			}
			break;
		}
		case ACTION_UNSET	:
			ret = Unset(ctid, param);
			if (param->save) {
				ret = vzctl_env_save(ctid);
				if (ret == 0)
					logger(0, 0, "Unset parameters for Container %s", ctid);
			}
			break;
		case ACTION_QUOTA	:
			break;
		case ACTION_MOUNT	:
		{
			ret = vzctl_env_mount(ctid, gparam->skip_fsck);
			break;
		}
		case ACTION_UMOUNT	:
			ret = vzctl_env_umount(ctid);
			break;
		case ACTION_DESTROY	:
		{
			ret = vzctl_env_destroy(ctid);
			if (ret == 0) {
				close(lckfd);
				lckfd = -1;
			}
			break;
		}
		case ACTION_REINSTALL	:
		{
			if (param->reinstall_list) {
				ret = reinstall_list(ctid, gparam);
			} else {
				char buf[4096];
				char *reinstall_scripts = NULL;
				char *reinstall_opts = NULL;

				if (param->reinstall_scripts) {
					List_genstr(param->reinstall_scripts, buf, sizeof(buf));
					reinstall_scripts = strdup(buf);
				}
				if (param->reinstall_opts) {
					List_genstr(param->reinstall_opts, buf, sizeof(buf));
					reinstall_opts = strdup(buf);
				}

				ret = vzctl_env_reinstall(ctid,
						param->skipbackup,
						param->resetpwdb,
						param->skipscripts,
						reinstall_scripts,
						reinstall_opts,
						param->ostmpl);
				free(reinstall_scripts);
				free(reinstall_opts);
			}
			break;
		}
		case ACTION_STATUS	:
			ret = Show(ctid);
			goto END;
		case ACTION_ENTER	:
			ret = vzctl_env_enter(ctid);
			goto END;
		case ACTION_CHKPNT	:
		case ACTION_SUSPEND	:
			ret = vzctl_env_chkpnt(ctid, gparam->cpt_cmd,
					gparam->dumpfile,
					gparam->cptcontext,
					gparam->cpu_flags,
					gparam->cpt_flags,
					gparam->skip_arpdetect);
			break;
		case ACTION_SNAPSHOT: {
			ret = vzctl_env_create_snapshot(ctid, param->snapshot_guid,
					param->snapshot_name, param->snapshot_desc,
					param->snapshot_flags);
			break;
		}
		case ACTION_SNAPSHOT_SWITCH:
			ret = vzctl_env_switch_snapshot(ctid, param->snapshot_guid,
					param->snapshot_flags);
			break;
		case ACTION_SNAPSHOT_DELETE:
			ret = vzctl_env_delete_snapshot(ctid, param->snapshot_guid);
			break;
		case ACTION_SNAPSHOT_MOUNT:
			ret = vzctl_env_mount_snapshot(ctid,
					param->target_dir,
					param->snapshot_guid);
			break;
		case ACTION_SNAPSHOT_UMOUNT:
			ret = vzctl_env_umount_snapshot(ctid, param->snapshot_guid);
			break;
		case ACTION_SNAPSHOT_LIST:
			ret = vzctl_env_snapshot_list(argc, argv, h);
			break;
		case ACTION_TSNAPSHOT:
			ret = vzctl_env_create_tsnapshot(ctid, param->snapshot_guid,
					param->snapshot_component_name);
			break;
		case ACTION_TSNAPSHOT_DELETE:
		       ret = vzctl_env_delete_tsnapshot(ctid, param->snapshot_guid);
		       break;
		case ACTION_RESTORE:
		       ret = vzctl_env_restore(ctid, gparam->cpt_cmd,
				       gparam->dumpfile,
				       gparam->cptcontext,
				       gparam->cpu_flags,
				       gparam->cpt_flags,
					get_flags(gparam));
		       break;
		case ACTION_EXEC	:
		case ACTION_EXEC2	:
		case ACTION_EXEC3	:
			ret = Exec(ctid, argv + 1, argc - 1, action,
					skiplock ? VZCTL_SKIP_LOCK : 0);
			goto END;
		case ACTION_EXECACTION	:
		{
			int i;
			const char *action;
			char **env = NULL;

			action = *(argv + 1);
			if (argv != NULL) {
				env = malloc(argc * sizeof(char*) +1);
				if (env == NULL) {
					ret = VZCTL_E_NOMEM;
					break;
				}
				for (i = 0; argv[i] != NULL; i++)
					env[i] = argv[i];
				env[i] = NULL;
			}
			ret = vzctl2_env_exec_action_script(h, action, env, 0, 0);

			free(env);
			break;
		}
		case ACTION_RUNSCRIPT	:
		{
			char *script;
			if (argc != 2) {
				logger(-1, 0, "Invalid number of arguments");
				break;
			}
			script = argv[1];
			ret = vz_run_script(ctid, script, NULL);
			break;
		}
		case ACTION_CONVERT	:
		{
			ret = vzctl_env_convert_layout(ctid, param->config,
					param->ve_layout);
			break;
		}
		case ACTION_UNREGISTER	:
		{
			int flags = (skipowner == YES ? VZ_REG_FORCE : 0);
			if (gparam->ignore_ha_cluster == YES)
				flags |= VZ_REG_SKIP_HA_CLUSTER;

			ret = vzctl2_env_unregister(NULL, ctid, flags);
			break;
		}
		case ACTION_CONSOLE:
		{
			if (start_console) {
				struct vzctl_console con = {.ntty = console_tty};

				ret = vzctl2_console_start(h, &con);
					if (ret)
				break;
			}
			ret = vzcon_attach(h, console_tty);
			break;
		}
		case ACTION_COMPACT:
		{
			static volatile int stop = 0;
			static dev_t compact_dev;
			struct vzctl_compact_param compact_param = {
					.defrag = gparam->compact_defrag,
					.threshold = gparam->compact_threshold,
					.delta = gparam->compact_delta,
					.dry = gparam->compact_dry,
					.stop = (int *)&stop,
					.compact_dev = &compact_dev,
			};
			ret = vzctl2_env_compact(h, &compact_param, sizeof(compact_param));
			break;
		}
		default :
			Usage();
			goto END;
	}

	if (skiplock != YES)
		vzctl2_env_unlock(h, lckfd);
END:

	if (!h)
		vzctl2_env_close(h);
	vzctl_set_log_quiet(1);
	char *commandLine = makecmdline(++argv_orig);
	logger(3, 0, "Command line: %s [%d]", commandLine, ret);
	free(commandLine);
	exit(ret);
}
