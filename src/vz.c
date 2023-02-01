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
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <linux/unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <limits.h>
#include <sys/personality.h>
#include <sys/mount.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include <grp.h>
#include <stdio.h>
#include <mntent.h>

#include "vzctl.h"
#include "config.h"
#include "script.h"
#include "tmplmn.h"
#include "util.h"
#include "util.h"
#include "ploop/libploop.h"
#include "clist.h"

static char * argv_bash[] = {"bash", NULL};
static char * envp_bash[] = {"HOME=/", "TERM=linux",
	"PATH=/bin:/sbin:/usr/bin:/usr/sbin",
	"SHELL=/bin/bash",
	NULL};

char **makeenv(char *const env[], struct CList *ve_env)
{
	char **penv;
	struct CList *l;
	int i;

	for (i = 0;  env != NULL && env[i] != NULL; i++);
	penv = (char **)malloc((i + ListSize(ve_env) + 1) * sizeof(char *));
	for (i = 0; env != NULL && env[i] != NULL; i++)
		penv[i] = strdup(env[i]);
	for (l = ve_env; l != NULL; l = l->next)
		penv[i++] = strdup(l->str);
	penv[i] = NULL;

	return penv;
}

int VZExecScript(ctid_t ctid, char *name, struct CList *env, int log, int timeout)
{
	int ret;
	char **penv;
	char *script, buf[STR_SIZE];
	char *dist;

	if (!(script = readscript(name, 1))) {
		logger(-1, 0, "Script %s is not found", name);
		return VZ_NOSCRIPT;
	}
	if (!env_is_running(ctid)) {
		 logger(-1, 0, "Unable to execute the script %s,"
			" the Container is not running", name);
		return VZ_VE_NOT_RUNNING;
	}
	if ((dist = get_dist_type(gparam)) != NULL) {
		snprintf(buf, sizeof(buf), "DIST=%s", dist);
		env = ListAddElem(env, buf, NULL, NULL);
	}
	logger(1, 0, "Run the script %s", name);
	penv = makeenv(envp_bash, env);
	ret = vzctl_env_exec(ctid, MODE_BASH_NOSTDIN,
			argv_bash, penv, script, timeout, EXEC_LOG_OUTPUT);
	freearg(penv);
	free(script);

	return ret ? VZCTL_E_SCRIPT_EXECUTION : 0;
}

int vz_run_script(ctid_t ctid, char *script, struct CList *env)
{
	int is_run, is_mount, ret, i;
	int wait_p[2], err_p[2];

	if (!stat_file(script)) {
		logger(-1, 0, "Script %s is not found: ", script);
		return -1;
	}
	if (pipe(wait_p) || pipe(err_p)) {
		logger(-1, errno, "Unable to create a pipe");
		return VZ_RESOURCE_ERROR;
	}
	if (!(is_mount = env_is_mounted(ctid))) {
		if ((ret = vzctl_env_mount(ctid, 0)))
			return ret;
	}
	if (!(is_run = env_is_running(ctid))) {
		/* TODO:
		if ((ret = VZCreate(ctid, param->ve_root, param->class_id,
			param, wait_p, err_p)))
		{
			return ret;
		}
		*/
		close(wait_p[0]); close(err_p[1]);
	}

	ret = VZExecScript(ctid, script, env, 1, 0);
	if (!is_run) {
		close(wait_p[1]); close(err_p[0]);
		for (i = 0; i < MAX_SHTD_TM; i++) {
			if (!env_is_running(ctid))
				break;
			usleep(500000);
		}
	}
	if (!is_mount)
		vzctl_env_umount(ctid);
	return ret;
}

int env_is_running(ctid_t ctid)
{
	return vzctl_env_is_run(ctid);
}

static int SetPasswd(ctid_t ctid, struct CList *userpw, int is_crypted)
{
	int ret;
	struct CList *p;
	char user[1024];
	char *passwd;
	int len;

	for (p = userpw; p != NULL; p = p->next) {
		if (p->str == NULL)
			continue;
		passwd = strchr(p->str, ':');
		if (passwd == NULL)
			continue;
		len = passwd - p->str;
		if (len >= sizeof(user))
			continue;
		strncpy(user, p->str, len);
		user[len] = '\0';
		ret = vzctl_env_set_userpasswd(ctid, user, ++passwd, is_crypted);
		if (ret)
			return ret;
	}
	return 0;
}

static char **get_env(void)
{
	char *tmp, *str, *token, *str2;
	char **env;
	struct CList *l = NULL;
	char buf[STR_SIZE];

	str = getenv(VZCTL_ENV);
	if (str == NULL)
		return NULL;
	tmp = strdup(str);
	if ((token = strtok(tmp, ":")) == NULL)
	{
		if ((str2 = getenv(str)) != NULL)
		{
			snprintf(buf, sizeof(buf), "%s=%s", str, str2);
			l = ListAddElem(l, buf, NULL, NULL);
		}
	} else {
		do {
			if ((str2 = getenv(token)) != NULL)
			{
				snprintf(buf, sizeof(buf), "%s=%s",
						token, str2);
				l = ListAddElem(l, buf, NULL, NULL);
			}
		} while ((token = strtok(NULL, ":")));
	}
	free(tmp);
	env = makeenv(envp_bash, l);
	ListFree(l);
	return env;
}

int Exec(ctid_t ctid, char **arg, int argc, int mode, int flags)
{
	int ret;
	char **env = get_env();
	
	ret = vzctl_env_exec(ctid,
			mode == ACTION_EXEC3 ? MODE_EXEC : MODE_BASH,
			arg, NULL, NULL, 0, flags);
	freearg(env);

	return (mode == ACTION_EXEC && ret) ? VZ_COMMAND_EXECUTION_ERROR : ret;
}

int Set_ve0(struct CParam *param)
{
	int ret = 0;
#if 0
	if (param->resetub) {
		ret = ub_configure(ctid, gparam);
	} else {
		if ((ret = ub_configure(ctid, param)))
			return ret;
		if (param->cpu_units &&
				(ret = vzctl_set_node_cpu_param(*param->cpu_units)))
			return ret;
	}
#endif
	return ret;
}

int Unset(ctid_t ctid, struct CParam *param)
{
	if (param->name)
		return vzctl_set_name(ctid, "");
	return 0;
}

static int fsremount(ctid_t ctid, const char *dst, int noatime)
{
	int mntopt = 0;

	if (check_var(gparam->ve_private, "Container private area is not set"))
		return VZ_VE_PRIVATE_NOTSET;
	mntopt |= MS_REMOUNT;
	if (noatime == YES)
		mntopt |= MS_NOATIME;

	if (mount("", dst, "ext4", mntopt, "")) {
		logger(-1, errno, "Cannot remount: %s", dst);
		return VZ_FS_CANTMOUNT;
	}
	return 0;
}

int Set(ctid_t ctid, struct CParam *param)
{
	int ret = 0;
	int is_mount;

	is_mount = env_is_mounted(ctid);

	ret = vzctl_set_name(ctid, param->name);
	if (ret)
		return ret;

	if (param->noatime)
	{
		if (is_mount)
		{
			logger(0, 0, "Setting the noatime mount flag for the Container ...");
			if ((ret = fsremount(ctid, gparam->ve_root, param->noatime)))
				return ret;
		}
		else if (!param->save)
		{
			logger(-1, 0, "Unable to change the "
					"atime mount flag: the Container is not mounted");
			return VZ_FS_NOT_MOUNTED;
		}
	}
	if (param->disk_param != NULL) {
		ret = vzctl_configure_disk(ctid, gparam->disk_op,
				param->disk_param, gparam->recreate,
				gparam->enc_flags);
		if (ret) {
			free(param->quota_block);
			param->quota_block = NULL;

			return ret;
		}

		/* Update DISKSPACE in case --size is specified for root image */
		if (gparam->disk_op == DEVICE_ACTION_SET && param->disk_param->size &&
				!strcmp(param->disk_param->uuid, DISK_ROOT_UUID))
		{
			if (param->quota_block == NULL)
				param->quota_block = malloc(sizeof(unsigned long) * 2);

			param->quota_block[0] = param->disk_param->size;
			param->quota_block[1] = param->disk_param->size;
		}
	}

	/* Set password */
	if (param->userpw != NULL)
	{
		ret = SetPasswd(ctid, param->userpw, param->is_crypted);
		if (ret)
			return ret;
	}

	if (param->netif_mac_renew)
		vzctl_renew_veth_mac(ctid, NULL);

	return vzctl_apply_param(ctid, param->setmode);

}

void Version(void)
{
	fprintf(stdout, "vzctl v." VERSION "\n");
}

void Usage(void)
{
	Version();
	fprintf(stdout, "\nUsage:\n"
"vzctl destroy | mount | umount | start | stop | status | enter | console\n"
"vzctl start <ctid|name> [--wait] [--repair]\n"
"vzctl create <ctid|name> [--ostemplate <name>]\n"
"	[--config <name>] [--private <path>] [--root <path>]\n"
"	[--ipadd <addr>] [--hostname <name>] [--skip_app_templates]\n"
"	[--force]\n"
"vzctl convert VEID\n"
"vzctl console VEID [1|2]\n"
"vzctl reinstall <ctid|name> [--skipbackup] [--resetpwdb]\n"
"vzctl suspend <ctid|name>\n"
"vzctl resume <ctid|name>\n"
"vzctl snapshot <ctid|name> [--id <uuid>] [--name <name>] [--description <desc>]\n"
"	[--skip-suspend]\n"
"vzctl snapshot-switch <ctid|name> --id <uuid>\n"
"vzctl snapshot-delete <ctid|name> --id <uuid>\n"
"vzctl snapshot-mount <ctid|name> --id <uuid> --target <path>\n"
"vzctl snapshot-umount <ctid|name> --id <uuid>\n"
"vzctl snapshot-list <ctid|name> [-H] [-o field[,field...]] [--id <uuid>]\n"
"vzctl exec | exec2 | exec3 <ctid|name> <command> [arg ...]\n"
"vzctl pause <ctid|name>\n"
"vzctl runscript <ctid|name> <script>\n"
"vzctl execaction <ctid|name> <action>\n"
"vzctl register <path> <ctid> [--force]\n"
"vzctl unregister <ctid|name>\n"
"vzctl monitor <ctid | 0>\n"
"vzctl compact <ctid|name> [--defrag] [--dry] [--threshold <value>] [--delta <value>]\n"
"vzctl set <ctid|name> [--save] [--ipadd <addr[/mask]> [--ipdel <addr>|all]\n"
"   [--hostname <name>] ]\n"
"   [--nameserver <addr>] [--searchdomain <name>] [--onboot yes|no]\n"
"   [--userpasswd <user>:<passwd> [--crypted]] [--cpuunits <N>] [--cpulimit <N>]\n"
"   [--cpus <N>] [--cpumask <{N[,N][.N-N]|all}>] [--nodemask <{N[,N][.N-N]|all}>]\n"
"   [--diskspace <soft>[:<hard>]] [--quotaugidlimit <N>] [--jquota <on|of>]\n"
"   [--rate <dev>:<class>:<Kbits>] [--ratebound yes|no]\n"
"   [--noatime yes|no] [--devnodes device:r|w|rw|none]\n"
"   [--pci_add [<domain>:]<bus>:<slot>.<func>] [--pci_del <d:b:s.f>]\n");
fprintf(stdout,
"   [--applyconfig <name>] [--setmode restart|ignore] [--description <desc>]\n"
"   [--netif_add <ifname[,mac,host_ifname,host_mac]]>] [--netif_del <ifname>]\n"
"   [--ifname <ifname> [--mac <mac>] [--host_ifname <name>] [--host_mac <mac>]\n"
"              [--ipadd <addr>] [--ipdel <addr>] [--gw <addr>] [--gw6 <addr>]\n"
"              [--dhcp <yes|no>] [--dhcp6 <yes|no>] [--network <id>]\n"
"              [--configure <none|all>]]\n"
"   [--bindmount_add [<src>]:<dst>[,nosuid,noexec,nodev]]\n"
"   [--bindmount_del <dst|all>] [--name <name>]\n"
"   [--netdev_add <name> [--netdev_del <name>]\n"
"   [--iptables <name>] [--disabled <yes|no>] [--ioprio <N>]\n"
"   [--iolimit <bytes/sec>] [--iopslimit <op/sec>] [--features name:on|off]\n"
"   [--mount_opts <opt[,opt]>] [--ha_enable <yes|no>] [--ha_prio <N>]\n"
"   [UBC parameters] [--ostemplate <name>] [--distribution <name>]\n"
"UBC parameters (N - items, P - pages, B - bytes):\n"
"Two numbers divided by colon denote barrier:limit.\n"
"In case the limit is not given it is set to the same value as the barrier.\n"
"   --numproc N[:N] 	--vmguarpages P[:P]\n"
"   --lockedpages P[:P]	--privvmpages P[:P]\n"
"   --shmpages P[:P]	--numfile N[:N]		--numflock N[:N]\n"
"   --numpty N[:N]	--numsiginfo N[:N]\n"
"   --numiptent N[:N]	--physpages P[:P]	--avnumproc N[:N]\n"
"   --swappages P[:P] --ram N --swap N --memguarantee <auto|N>\n"
"   --numnetif N --pagecache-isolation=<yes|no>\n");
if (is_vswap_mode())
	fprintf(stdout,
"   --vm_overcommit N\n");
}
