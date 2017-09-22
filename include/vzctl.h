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

/* $Id$ */
#ifndef _VZCTL_BASE_H_
#define _VZCTL_BASE_H_

#include <vzctl/vzctl_param.h>
#include <vzctl/libvzctl.h>

#include "clist.h"

/* libs include */
#include "list.h"
#include "vzerror.h"

#define MAXCPUUNITS		500000
#define MINCPUUNITS		8
#define UNLCPUUNITS		1000
#define HNCPUUNITS		1000
#define LHTCPUUNITS		250
#define MIN_NUMIPTENT		16

#define DEBUG_LEVEL		2
#define MAXATTEMPT		10
#define SCRIPT_DIR		"/usr/libexec/libvzctl/scripts/"
#define DIST_CONF_DIR		SCRIPT_DIR"dists/"
#define DIST_SCRIPTS		DIST_CONF_DIR "scripts/"
#define	DIST_CONF_DEF		"default"
#define	VE_FUNC			DIST_SCRIPTS "functions"
#define VZCTL_VE_CONF		"ve.conf"

#define NET_ADD			"vz-net_add"
#define NET_DEL                 "vz-net_del"

#define REINSTALL_OLD_MNT	"/mnt"
#define CUSTOM_SCRIPT_DIR	"/etc/vz/reinstall.d"

#define	VZPKG			"/usr/sbin/vzpkg"
#define	VZCACHE			"/usr/sbin/vzcache"

#define MOUNT			"/sbin/mount"
#define MOUNT_ROOT		"/sbin/mount_union"
#define MOUNT_DEV		"/sbin/mount_devfs"

#define VZCTLDEV		"/dev/vzctl"
#define VEINFOPATH		"/proc/vz/veinfo"
#define VEINFOPATHOLD		"/proc/veinfo"
#define PROCMOUNTS		"/proc/mounts"
#define PROCCPU			"/proc/cpuinfo"
#define PROCUBC			"/proc/user_beancounters"
#define PROCTHR			"/proc/sys/kernel/threads-max"
#define PROCMEM			"/proc/meminfo"
#define PROCVEIP		"/proc/vz/veip"
#define VZNETCFG		"/usr/sbin/vznetcfg"

#define QUOTA_EXPTIME		259200
#define CFG_FILE		"/etc/vz/vz.conf"

#define ACTIONLOCKFILE		".lck"
#define SEQFILE			"vzctl.seq"

#define VE_ROOT_DEFAULT		"/vz/root"
#define VE_PRIVATE_DEFAULT	"/vz/private"
#define TEMPLATE_DEFAULT	"/vz/template"

#define VE_STATE_DIR		"/var/vz/veip"
#define VZFIFO_FILE		"/.vzfifo"

#define START_PREFIX		"start"
#define STOP_PREFIX		"stop"
#define MOUNT_PREFIX		"mount"
#define UMOUNT_PREFIX		"umount"
#define DESTROY_PREFIX		"destroy"
#define DESTR_PREFIX		"destroyed"
#define SAMPLE_CONF		"ve-vps.basic.conf-sample"

#define DISK_ROOT_UUID		"{00000000-0000-0000-0000-000000000000}"

#define VZCTL_SNAPSHOT_SKIP_DUMP	0x01
#define VZCTL_SNAPSHOT_SKIP_RESUME	0x02

#define MAX_SHTD_TM	60

#define DISKSPACE_INK		100
#define DISKINODE_INK		100

#define VE0ID			2147483647

#define QUOTA_INIT		1
#define QUOTA_SET		2
#define QUOTA_OFF		3
#define QUOTA_ON		4
#define QUOTA_DROP		5
#define QUOTA_STAT		6
#define QUOTA_SETPRVT		7
#define QUOTA_STAT2		8
#define QUOTA_MARKDURTY		9
#define QUOTA_SHOW		10

#define MODE	0755

#define VE_OLDDIR		"/old"

#define STRSIZ	1024
/* Action numeration should begin with 1. Value 0 is reserved. */
#define ACTION_CREATE		1
#define ST_CREATE		"creating"
#define ACTION_SETUP		2
#define ACTION_DESTROY		3
#define ST_DESTROY		"destroying"
#define ACTION_MOUNT		4
#define ST_MOUNT		"mounting"
#define ACTION_UMOUNT		6
#define ST_UMOUNT		"unmounting"
#define ACTION_START		7
#define ST_START		"starting"
#define ACTION_STOP		8
#define ST_STOP			"stopping"
#define ACTION_RESTART		9
#define ST_RESTART		"restarting"
#define ACTION_SET		10
#define ST_SET			"setting"
#define ACTION_STATUS		11
#define ACTION_EXEC		12
#define ACTION_EXEC2		13
#define ACTION_EXEC3		14
#define ACTION_ENTER		15
#define ACTION_QUOTA		16
#define ST_QUOTA_INIT		"initializing-quota"
#define ACTION_REINSTALL	19
#define ST_REINSTALL		"reinstalling"
#define ST_UPDATING		"updating"
#define ACTION_EXEC_NOSTDIN	21
#define	ACTION_RUNSCRIPT	22
#define ACTION_CHKPNT		23
#define ACTION_RESTORE		24
#define ST_RESUME		"resuming"
#define ST_RESTORE		"restoring"
#define ACTION_SUSPEND		25
#define ST_SUSPEND		"suspending"
#define ACTION_MONITOR		32
#define ACTION_EXECACTION	33
#define ACTION_REGISTER		34
#define ACTION_UNREGISTER	35
#define ACTION_CONVERT		36
#define ACTION_UNSET		37
#define ACTION_ATTACH		38
#define ACTION_SET_RATE		39
#define ACTION_SNAPSHOT		40
#define ST_SNAPSHOT		"snapshoting"
#define ACTION_SNAPSHOT_SWITCH	41
#define ST_SNAPSHOT_SWITCH	"switching snapshot"
#define ACTION_SNAPSHOT_DELETE  42
#define ST_SNAPSHOT_DELETE	"deleting snapshot"
#define ACTION_SNAPSHOT_LIST	43
#define ACTION_CONSOLE		44
#define ACTION_SNAPSHOT_MOUNT	45
#define ACTION_SNAPSHOT_UMOUNT	46
#define ACTION_STOP_FORCE	47
#define ACTION_TSNAPSHOT	48
#define ACTION_TSNAPSHOT_DELETE	49
#define ACTION_PAUSE		50

#define PARAM_DUMMY		0
#define PARAM_KMEMSIZE		'k'
#define PARAM_LOCKEDPAGES	'l'
#define PARAM_PRIVVMPAGES	1
#define PARAM_TOTVMPAGES	'v'
#define PARAM_SHMPAGES		2
#define PARAM_ANONSHPAGES	'z'
#define PARAM_IPCSHMPAGES	'm'
#define PARAM_NUMPROC		'p'
#define PARAM_PHYSPAGES		3
#define PARAM_RSSPAGES		'r'
#define PARAM_VMGUARPAGES	4
#define PARAM_OOMGUARPAGES	5
#define PARAM_OOMGUAR		'g'
#define PARAM_NUMTCPSOCK	6
#define PARAM_NUMSOCK		's'
#define PARAM_NUMFLOCK		'f'
#define PARAM_NUMPTY		't'
#define PARAM_NUMSIGINFO	'i'
#define PARAM_TCPSNDBUF		7
#define PARAM_TCPSENDBUF	'a'
#define PARAM_TCPRCVBUF		'b'
#define PARAM_OTHERSOCKBUF	8
#define PARAM_UNIXSOCKBUF	'u'
#define PARAM_DGRAMRCVBUF	9
#define PARAM_SOCKRCVBUF	'w'
#define PARAM_NUMOTHERSOCK	10
#define PARAM_NUMUNIXSOCK	'd'
#define PARAM_IPTENTRIES	'e'
#define PARAM_NUMFILE		'n'
#define PARAM_DCACHE		'x'


/* Create VZFS private area options */
#define PARAM_PKGVER		11
#define PARAM_PKGSET		12
#define PARAM_OSTEMPLATE	13

#define PARAM_CPUWEIGHT		VZCTL_PARAM_CPUWEIGHT
#define PARAM_CPULIMIT		VZCTL_PARAM_CPULIMIT
#define PARAM_CPUUNITS		VZCTL_PARAM_CPUUNITS

#define PARAM_IP_ADD		18
#define PARAM_IP_DEL		19
#define PARAM_HOSTNAME		VZCTL_PARAM_HOSTNAME
#define PARAM_SEARCHDOMAIN	VZCTL_PARAM_SEARCHDOMAIN
#define PARAM_NAMESERVER	VZCTL_PARAM_NAMESERVER
#define PARAM_USERPW		24
#define PARAM_TEMPLATE		25
#define PARAM_PW_CRYPTED	26
#define PARAM_CLASSID		27
#define PARAM_IP		28
#define PARAM_SAVE		'S'
#define PARAM_ROOT		32
#define PARAM_PRIVATE		33
#define PARAM_ONBOOT		VZCTL_PARAM_ONBOOT

#define PARAM_QUOTA_BLOCK	36
#define PARAM_QUOTAUGIDLIMIT	VZCTL_PARAM_QUOTAUGIDLIMIT
#define PARAM_QUOTA_INODE	38
#define PARAM_QUOTA_TIME	40

#define PARAM_SLM_MEMORY_LIMIT	149
#define PARAM_SLM_MODE		151
#define PARAM_SLMPATTERN	152

#define PARAM_RATE		VZCTL_PARAM_RATE
#define PARAM_NOATIME		43
#define PARAM_SAFE		44
#define PARAM_CAP		45
#define PARAM_RATEBOUND		VZCTL_PARAM_RATEBOUND
#define PARAM_CONFIG		47
#define PARAM_VETYPE		48
#define PARAM_CFGVER		50
#define PARAM_DEVICES		VZCTL_PARAM_DEVICES
#define PARAM_AVNUMPROC		52
#define PARAM_TEMPLATES		93
#define PARAM_DEVNODES		VZCTL_PARAM_DEVNODES
#define	PARAM_SKIPQUOTACHECK	95

#define PARAM_SKIPBACKUP	300
#define PARAM_RESETPWDB		301
#define PARAM_FAST		310
#define PARAM_SKIP_UMOUNT	311
#define PARAM_EXPTIME		312
#define PARAM_SKIP_APP		313
#define	PARAM_APPCONF		314
#define PARAM_IPTABLES		VZCTL_PARAM_IPTABLES
#define PARAM_NETDEV_ADD	VZCTL_PARAM_NETDEV
#define PARAM_NETDEV_DEL	VZCTL_PARAM_NETDEV_DEL
#define PARAM_DISABLED		319
#define	PARAM_FORCE		320
#define PARAM_SKIP_VE_SETUP	321
#define PARAM_CONFIG_SAMPLE	322
#define PARAM_CONFIG_CUSTOMIZE	323
#define PARAM_SETMODE		324
#define PARAM_DUMPFILE		325
#define PARAM_CPTCONTEXT	326
#define PARAM_CPU_FLAGS		327
#define PARAM_KILL		328
#define PARAM_RESUME		329
#define PARAM_OSTEMPLATE_STR	330
#define PARAM_WAIT		331
#define PARAM_SKIP_ARPDETECT	333
#define PARAM_DESCRIPTION	334
#define PARAM_REINSTALL_SCRIPTS	335
#define PARAM_REINSTALL_LIST	336
#define PARAM_DUMP		337
#define PARAM_UNDUMP		338
#define PARAM_SUSPEND		339
#define PARAM_RESETUB		341
#define PARAM_NAME		345
#define PARAM_REINSTALL_DESC	346
#define PARAM_NETIF_MAC_RENEW	350
#define PARAM_DEF_GW		361
#define PARAM_BINDMOUNT_ADD	362
#define PARAM_BINDMOUNT_DEL	363
#define PARAM_BR_CPU_AVG_USAGE	364
#define PARAM_BR_CPULIMIT	365
#define PARAM_APPCONF_MAP	366
#define PARAM_VE_LAYOUT		369
#define PARAM_SKIPSCRIPTS	371
#define PARAM_VZPKG_OPTS	372
#define PARAM_SKIP_CLUSTER	374
#define PARAM_RENEW		375
#define PARAM_SWAPPAGES		377
#define PARAM_EXT_IP_ADD	378
#define PARAM_EXT_IP_DEL	379
#define PARAM_SWAP		381
#define PARAM_MEMORY		382
#define PARAM_VM_OVER		383
#define PARAM_OSRELEASE		385
#define PARAM_EXPANDED		393
#define PARAM_PREALLOCATED	394
#define PARAM_RAW		395
#define PARAM_MOUNT_OPTS	396
#define PARAM_KEEP_PAGES	397
#define PARAM_SNAPSHOT_GUID	398
#define PARAM_SNAPSHOT_NAME	399
#define PARAM_SNAPSHOT_DESC	400
#define PARAM_ENV_ARGS		401
#define PARAM_SNAPSHOT_SKIP_DUMP	402
#define PARAM_UNFREEZE		403
#define PARAM_GUID		404
#define PARAM_APPLY_IPONLY	406
#define PARAM_TARGET_DIR	407
#define PARAM_DEVICE_ADD	409
#define PARAM_DEVICE_DEL	410
#define PARAM_DEVICE_SET	411
#define PARAM_SIZE		412
#define PARAM_IMAGE		413
#define PARAM_DEVICE_UUID	414
#define PARAM_ENABLE		415
#define PARAM_DISABLE		416
#define PARAM_DISK_MNT		419
#define PARAM_REG_START		421
#define PARAM_DISK_OFFLINE	422
#define PARAM_CPT_STOP_TRACKER	423
#define PARAM_SKIP_RESUME	424
#define PARAM_SNAPSHOT_COMPONENT_NAME	425
#define PARAM_DISK_DETACH	426
#define PARAM_SKIP_FSCK		428
#define PARAM_NO_HDD		431
#define PARAM_DEVICE		432
#define PARAM_CPT_CREATE_DEVMAP	433
#define PARAM_DISK_STORAGE_URL	434
#define PARAM_ENC_KEYID		435
#define PARAM_ENC_REENCRYPT	436
#define PARAM_ENC_WIPE		437
#define PARAM_RECREATE		438

/* parsing template for getopt, based on PARAM values */
#define PARAM_LINE "e:p:s:f:t:i:r:v:g:c:l:m:z:k:a:b:u:w:d:n:x:hS"

#define VE_CPULIMIT_DEFAULT	0
#define VE_CPUWEIGHT_DEFAULT	1

#define LOG_FILE_DEFAULT	"/var/log/vzctl.log"
#define VE_OUT		-1
#define ERR_SUCCESS	0

#define MAX_STR		16384
#define STR_SIZE	512
#define ENVRETRY	20

#define MOVE		0
#define DESTR		1
#define VE_TYPE_REGULAR		"regular"
#define VE_TYPE_TEMPLATE	"template"

#define REGULAR_VE	0
#define SHARED_VE	1
#define SHARED_BASE_VE	2
#define	RECOVER_VE	3
#define	REINSTALL_VE	4

#define VZCTL_ENV	"VZCTL_ENV"
#define DEF_CLASSID	2

#define SLM_MODE_UBC	1
#define SLM_MODE_SLM	2
#define SLM_MODE_ALL	3

#define YES		VZCTL_PARAM_ON
#define NO		VZCTL_PARAM_OFF
#define CFG_DIR		VZ_ENV_CONF_DIR
#define ENV_CONF_DIR	VZ_ENV_CONF_DIR
#define GLOBAL_CFG	VZ_GLOBAL_CFG
#define VZCTL_VE_FS_DIR	"/fs"

#define LOCALE_UTF8	"UTF-8"
#define DEF_DUMPDIR	"/vz/tmp"
#define DEF_DUMPFILE	"Dump.%d"
#define VZCTL_VE_DUMP_DIR       "/dump"
#define VZCTL_VE_DUMP_FILE      "Dump"

#define SCRIPT_D_DIR            "/etc/vz/script.d/"

#define ADD             0
#define DEL             1
#define DELALL          3

/* Parse errro codes */
#define ERR_DUP         -1
#define ERR_INVAL       -2
#define ERR_UNK         -3
#define ERR_NOMEM       -4
#define ERR_OTHER       -5
#define ERR_INVAL_SKIP  -6
#define ERR_LONG_TRUNC  -7

// Netwok operations
#define IP_EXT_ADD      8
#define IP_EXT_DEL      9

enum {
	TM_UNKNOWN,
	TM_ST,
	TM_EZ,
};

enum {
	APPCONF_MAP_ALL = 0x01,
	APPCONF_NAME	= 0x02,
};

enum {
	MM_UBC = 0,
	MM_SLM,
	MM_VSWAP,
};

struct cpu_limit {
	float limit;
	int type;
	unsigned long long val;
};

enum {
	DEVICE_ACTION_UNSPEC =  0,
	DEVICE_ACTION_ADD,
	DEVICE_ACTION_DEL,
	DEVICE_ACTION_SET,
	DEVICE_ACTION_DETACH,
};

enum {
	EXEC_STD_REDIRECT = (1 << 16),  /* redirect std[in,out,err] from/to VE */
	EXEC_OPENPTY = (1 << 17),       /* emulate pty in VE*/
	EXEC_LOG_OUTPUT = (1 << 18),
};

struct vzctl_env_features {
	unsigned long long mask;
	unsigned long long known;
};
typedef struct vzctl_env_features vzctl_env_features_t;


typedef unsigned int  cap_t;
struct vzctl_env_features;

struct CParam
{
	int fastkill;
	int save;
	int skiplock;
	char *applyconfig;
	int applyconfig_map;
	int version;
	char *ve_root_orig;
	char *ve_root;
	char *ve_private_orig;
	char *ve_private;
	char *ve_private_fs;
	struct CList *templates;
	char *ostmpl_str;
	char *ostmpl;
	struct CList *sup_tech;
	char *dist;
	char *quota_db_path;
	char *log_file;
	char *action_lck;
	char *lockdir;
	char *config;
	char *validatemode;
	int quiet;
	int use_quota;
	int skip_app_templates;
	int skip_ve_setup;
	int wait;
	int resetub;

/* Host parameters */
	unsigned long *class_id;
	struct CList *ipadd;
	struct CList *ext_ipadd;
	struct CList *ipdel;
	struct CList *ext_ipdel;
	int ipv6;
	int ipdelall;
	int ext_ipdelall;
	int skip_arpdetect;
	char *vzdev;
	struct CList *nameserver;
	struct CList *searchdomain;
	char *hostname;
	char *description;
	char *default_gw;
	struct CList *userpw;
	int is_crypted;
	unsigned long *br_cpu_avg_usage;
	unsigned long *br_cpulimit;

/* Disk quota parameters */
	unsigned long *quota_block;
	unsigned long *quota_inode;
	unsigned long *quota_time;
	unsigned long *quotaugidlimit;
/* Mount options */
	int noatime;
/* Migration and Suspend */
	char *dumpfile;
	char *dumpdir;
	unsigned int cptcontext;
	unsigned int cpu_flags;
	int cpt_cmd;
	int cpt_flags;
/* Destroy option */
	int del_safe;
/* Shared */
	int skipbackup;
	int skipscripts;
	int resetpwdb;
	struct CList *reinstall_scripts;
	struct CList *reinstall_opts;
	int reinstall_list;
	int reinstall_desc;
/* VE env */
	struct CList *ve_env;
	int start_disabled;
	int start_force;
	int force;
/* Config */
	int setmode;
	int netif_mac_renew;
/* DISK */
	int disk_op;
	struct vzctl_disk_param *disk_param;
	int no_hdd;

/* NAME */
	char *name;
/* Features */
	int ve_layout;
	int ve_layout_def;
	int ploop_type;
	char *osrelease;
	int vetype;
	char *snapshot_guid;
	char *snapshot_name;
	char *snapshot_desc;
	int snapshot_flags;
	char *snapshot_component_name;
	char *guid;
	int apply_iponly;
	int stop_flags;
	char *target_dir;
	int ha_enable;
	unsigned long *ha_prio;
	int skip_fsck;
	char *enc_keyid;
	int enc_flags;
	int recreate;
};

extern struct CParam *gparam;

int monitoring(ctid_t ctid);
int parse_ip(char *str, char **ipstr, unsigned int *mask);
int vzctl_configure_disk(ctid_t ctid, int op, struct vzctl_disk_param *param,
		int recreate, int flags);
int VZExecScript(ctid_t ctid, char *name, struct CList *env, int log, int timeout);
int env_is_running(ctid_t ctid);
int Exec(ctid_t ctid, char **arg, int argc, int mode);
int Set(ctid_t ctid, struct CParam *param);
int Unset(ctid_t ctid, struct CParam *param);
int Set_ve0(struct CParam *param);
int vz_run_script(ctid_t ctid, char *script, struct CList *env);
int vzctl_env_create_snapshot(ctid_t ctid, char *guid,
		char *name, char *desc, int flags);
int vzctl_env_create_tsnapshot(ctid_t ctid, char *guid, char *component_name);
int vzctl_env_switch_snapshot(ctid_t ctid, const char *guid, int flags);
int vzctl_env_delete_snapshot(ctid_t ctid, const char *guid);
int vzctl_env_delete_tsnapshot(ctid_t ctid, const char *guid);
int vzctl_env_mount_snapshot(ctid_t ctid, const char *mnt, const char *guid);
int vzctl_env_umount_snapshot(ctid_t ctid, const char *guid);
int vzctl_env_snapshot_list(int argc, char **argv, struct vzctl_env_handle *h);
int vzctl_env_enter(ctid_t ctid);
int vzctl_env_start(ctid_t ctid, int skip_action_script, int wait,
		int skip_ve_setup, int skip_fsck);
int vzctl_env_pause(ctid_t ctid);
int vzctl_env_restart(ctid_t ctid, int wait, int skip_ve_setup);
int vzctl_env_stop(ctid_t ctid, int stop_mode, int flags);
int vzctl_env_set_userpasswd(ctid_t ctid, const char *user, const char *passwd, int is_crypted);
int vzctl_env_mount(ctid_t ctid, int skip_fsck);
int vzctl_env_umount(ctid_t ctid);
int vzctl_set_cpu_param(ctid_t ctid,
		unsigned long *vcpus,
		unsigned long *weight,
		unsigned long *units,
		struct cpu_limit *limit,
		struct vzctl_cpumask *cpumask,
		struct vzctl_nodemask *nodemask);
int vzctl_set_node_cpu_param(unsigned long units);
int vzctl_env_convert_layout(ctid_t ctid, const char *ve_conf, int new_layout);
int vzctl_env_set_rate(ctid_t ctid);
int vzctl_add_env_param_by_name(ctid_t ctid, const char *name, const char *str);
int vzctl_add_env_param_by_id(ctid_t ctid, int id, const char *str);
int vzctl_env_destroy(ctid_t ctid);
int vzctl_env_is_run(ctid_t ctid);
int vzctl_env_save(ctid_t ctid);
int vzctl_set_name(ctid_t ctid, const char *name);
int vzctl_env_open_apply_conf(ctid_t ctid, const char *conf);
int vzctl_env_create(ctid_t ctid,
		char *uuid,
		char *config,
		char *ostmpl,
		char *ve_private,
		char *ve_root,
		char *name,
		char *enc_keyid,
		int no_hdd,
		int layout);
int vzctl_env_reinstall(ctid_t ctid,
		int skipbackup,
		int resetpwdb,
		int skipscripts,
		char *reinstall_scripts,
		char *reinstall_opts,
		char *ostemplate);
int vzctl_env_chkpnt(ctid_t ctid, int cmd,
		char *dumpfile,
		unsigned ctx,
		unsigned int cpu_flags,
		int cpt_flags,
		int skip_arpdetect);
int vzctl_env_restore(ctid_t ctid, int cmd,
		char *dumpfile,
		unsigned ctx,
		unsigned int cpu_flags,
		int cpt_flags,
		int skip_arpdetect,
		int skip_fsck);
int vzctl_lib_init(void);
int vzctl_apply_param(ctid_t ctid, int setmode);
int vzctl_del_param_by_id(ctid_t ctid, int id);
int vzctl_init_log(ctid_t ctid, int quiet, const char *progname);
void vzctl_set_log_level(int level);
int vzctl_set_log_enable(int enable);
int vzctl_set_log_quiet(int quiet);
int vzctl_get_log_quiet(void);
int vzctl_set_log_verbose(int quiet);
int vzctl_get_vzctlfd(void);
int vzctl_env_exec(ctid_t ctid, int exec_mode,
                char *const argv[], char *const envp[], char *std_in, int timeout, int flags);
void vzctl_env_close(void);
int vzctl_env_layout_version(const char *path);
int vzctl_convertstr(const char *src, char *dst, int dst_size);
int vzctl_open(void);
void vzctl_close(void);
int vzctl_renew_veth_mac(ctid_t ctid, const char *ifname);
int vzctl_get_env_status(ctid_t ctid, vzctl_env_status_t *status, int mask);
struct vzctl_env_handle *vzctl_env_open(ctid_t ctid, const char *conf,
		int flags, int *ret);
extern void vzctl2_log(int level, int err_no, const char *format, ...)
        __attribute__ ((__format__ (__printf__, 3, 4)));

#define logger vzctl2_log
int vzctl_err(int err, int eno, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 3, 4)));

int get_vzctlfd(void);
int vzcon_attach(struct vzctl_env_handle *h, int ntty);
#endif
