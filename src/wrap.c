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
#include <string.h>
#include <vzctl/libvzctl.h>
#include <limits.h>
#include <uuid/uuid.h>

#include "vzctl.h"
#include "clist.h"
#include "vzerror.h"
#include "vzerror.h"

extern char *get_ipname(unsigned int ip);

static struct vzctl_env_handle *_g_h = NULL;
static struct vzctl_env_handle *_g_apply_conf;
static struct vzctl_env_param *_g_env_param;
static ctid_t _g_eid;

struct vzctl_env_param *get_env_param(ctid_t ctid)
{
	if (_g_env_param == NULL)
		_g_env_param = vzctl2_alloc_env_param();

	return _g_env_param;
}

struct vzctl_env_handle *vzctl_env_open(ctid_t ctid, const char *conf,
		int flags, int *ret)
{
	struct vzctl_env_handle *h;

	if (CMP_CTID(ctid, _g_eid) == 0 && _g_h != NULL)
		return _g_h;

	if (conf)
		h = vzctl2_env_open_conf(ctid, conf, flags, ret);
	else
		h = vzctl2_env_open(ctid, flags, ret);
	if (h == NULL)
		return NULL;
	_g_h = h;
	SET_CTID(_g_eid, ctid);

	return _g_h;
}

void vzctl_env_close(void)
{
	if (_g_h != NULL) {
		vzctl2_env_close(_g_h);
		_g_h = NULL;
	}
}

int vzctl_env_unregister(const char *path, ctid_t ctid, int flags)
{
	return vzctl2_env_unregister(path, ctid, flags);
}

vzctl_ids_t *vzctl_alloc_env_ids(void)
{
	return vzctl2_alloc_env_ids();
}

void vzctl_free_env_ids(vzctl_ids_t *veids)
{
	return vzctl2_free_env_ids(veids);
}

int vzctl_get_env_ids_by_state(vzctl_ids_t *veids, unsigned int mask)
{
	return vzctl2_get_env_ids_by_state(veids, mask);
}

int vzctl_get_env_status(ctid_t ctid, vzctl_env_status_t *status, int mask)
{
	return vzctl2_get_env_status(ctid, status, mask);
}

int vzctl_env_layout_version(const char *path)
{
	return vzctl2_env_layout_version(path);
}

int vzctl_get_free_env_id(unsigned int *newid)
{
	return vzctl2_get_free_envid(newid, 0, 0);
}

int env_is_mounted(ctid_t ctid)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_is_mounted(h);
}

int vzctl_mount_disk_image(const char *path, struct vzctl_mount_param *param)
{
	return vzctl2_mount_disk_image(path, param);
}

int vzctl_mount_image(const char *ve_private, struct vzctl_mount_param *param)
{
	return vzctl2_mount_image(ve_private, param);
}

int vzctl_umount_disk_image(const char *path)
{
	return vzctl2_umount_disk_image(path);
}

int vzctl_umount_image(const char *ve_private)
{
	return vzctl2_umount_image(ve_private);
}

int vzctl_create_disk_image(const char *path, struct vzctl_create_image_param *param)
{
	return vzctl2_create_disk_image(path, param);
}

int vzctl_create_root_image(const char *ve_private, struct vzctl_create_image_param *param)
{
	return vzctl2_create_root_image(ve_private, param);
}

int vzctl_create_image(const char *ve_private, struct vzctl_create_image_param *param)
{
	return vzctl2_create_image(ve_private, param);
}

int vzctl_resize_disk_image(const char *path, unsigned long long newsize, int offline)
{
	return vzctl2_resize_disk_image(path, newsize, offline);
}

int is_image_mounted(const char *path)
{
	return vzctl2_is_image_mounted(path);
}

int vzctl_convert_image(const char *ve_private, int mode)
{
	return vzctl2_convert_image(ve_private, mode);
}

int vzctl_get_ploop_dev_by_mnt(const char *mnt, char *out, int len)
{
	return vzctl2_get_ploop_dev_by_mnt(mnt, out, len);
}

int vzctl_get_ploop_dev(const char *path, char *dev, int len)
{
	return vzctl2_get_ploop_dev(path, dev, len);
}

int vzctl_get_top_image_fname(char *ve_private, char *out, int len)
{
	return vzctl2_get_top_image_fname(ve_private, out, len);
}

int vzctl_delete_disk_snapshot(const char *path, const char *guid)
{
	return vzctl2_delete_disk_snapshot(path, guid);
}

int vzctl_merge_disk_snapshot(const char *path, const char *guid)
{
	return vzctl2_merge_disk_snapshot(path, guid);
}

int vzctl_create_disk_snapshot(const char *path, const char *guid)
{
	return vzctl2_create_disk_snapshot(path, guid);
}

int vzctl_switch_disk_snapshot(const char *path, const char *guid, const char *guid_old, int flags)
{
	return vzctl2_switch_disk_snapshot(path, guid, guid_old, flags);
}

int vzctl_lock(const char *lockfile, int mode, unsigned int timeout)
{
	return vzctl2_lock(lockfile, mode, timeout);
}

void vzctl_unlock(int fd, const char *lockfile)
{
	vzctl2_unlock(fd, lockfile);
}

int vzctl_init_log(ctid_t ctid, int quiet, const char *progname)
{
	int ret;

	ret = vzctl2_init_log(progname);
	if (ret)
		return ret;

	vzctl2_set_ctx(ctid);
	vzctl2_set_log_quiet(quiet);

	return 0;
}

const char *vzctl_get_last_error(void)
{
	return vzctl2_get_last_error();
}

int vzctl_set_log_file(const char *file)
{
	return vzctl2_set_log_file(file);
}

int vzctl_get_log_fd(void)
{
	return vzctl2_get_log_fd();
}

void vzctl_set_log_level(int level)
{
	vzctl2_set_log_level(level);
}

int vzctl_set_log_enable(int enable)
{
	return vzctl2_set_log_enable(enable);
}

int vzctl_set_log_quiet(int quiet)
{
	return vzctl2_set_log_quiet(quiet);
}

int vzctl_get_log_quiet(void)
{
	return vzctl2_get_log_quiet();
}

int vzctl_set_log_verbose(int verbose)
{
	return vzctl2_set_log_verbose(verbose);
}

int vzctl_get_cpuinfo(struct vzctl_cpuinfo *info)
{
	return vzctl2_get_cpuinfo(info);
}

int vzctl_register_evt(vzevt_handle_t **h)
{
	return vzctl2_register_evt(h);
}

void vzctl_unregister_evt(vzevt_handle_t *h)
{
	return vzctl2_unregister_evt(h);
}

int vzctl_get_evt_fd(vzevt_handle_t *h)
{
	return vzctl2_get_evt_fd(h);
}

int vzctl_get_state_evt(vzevt_handle_t *h, struct vzctl_state_evt *evt, int size)
{
	return vzctl2_get_state_evt(h, evt, size);
}

int vzctl_send_state_evt(ctid_t ctid, int state)
{
	return vzctl2_send_state_evt(ctid, state);
}

int vzctl_env_create_snapshot(ctid_t ctid, char *guid, char *name, char *desc, int flags)
{
	int ret;
	struct vzctl_snapshot_param param = {
		.guid = guid,
		.name = name,
		.desc = desc,
		.flags = flags,
	};
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_create_snapshot(h, &param);
}

int vzctl_env_create_tsnapshot(ctid_t ctid, char *guid, char *component_name)
{
	int ret;
	struct vzctl_tsnapshot_param tsnap = {
		.component_name = component_name,
	};
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_create_tsnapshot(h, guid, &tsnap, NULL);
}

int vzctl_env_switch_snapshot(ctid_t ctid, const char *guid, int flags)
{
	int ret;
	struct vzctl_env_handle *h;
	struct vzctl_switch_snapshot_param param = {
		.guid = (char *)guid,
		.flags = flags
	};

	h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_switch_snapshot(h, &param);
}

int vzctl_env_delete_snapshot(ctid_t ctid, const char *guid)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_delete_snapshot(h, guid);
}

int vzctl_env_delete_tsnapshot(ctid_t ctid, const char *guid)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_delete_tsnapshot(h, guid, NULL);
}

int vzctl_env_mount_snapshot(ctid_t ctid, const char *mnt, const char *guid)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_mount_snapshot(h, mnt, guid);
}

int vzctl_env_umount_snapshot(ctid_t ctid, const char *guid)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_umount_snapshot(h, guid);
}

int vzctl_lib_init(void)
{
	return vzctl2_lib_init();
}

void vzctl_lib_close(void)
{
	return vzctl2_lib_close();
}

int vzctl_get_vzctlfd(void)
{
	return vzctl2_get_vzctlfd();
}

void vzctl_set_flags(unsigned int flags)
{
	vzctl2_set_flags(flags);
}

int vzctl_open(void)
{
	vzctl_set_flags(0);
	return vzctl_lib_init();
}

int get_vzctlfd(void)
{
	return vzctl_get_vzctlfd();
}

void vzctl_close(void)
{
	vzctl_lib_close();
}

void vzctl_set(unsigned int flags)
{
	vzctl_set_flags(flags);
}

int vzctl_env_start(ctid_t ctid, int flags)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_start(h, flags);
}

int vzctl_env_pause(ctid_t ctid)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_pause(h, 0);
}

int vzctl_env_restart(ctid_t ctid, int wait, int skip_ve_setup)
{
	int ret;
	int flags = (wait ? VZCTL_WAIT : 0) |
			(skip_ve_setup ? VZCTL_SKIP_SETUP : 0) ;

	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_restart(h, flags);
}

int vzctl_env_stop(ctid_t ctid, int stop_mode, int flags)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_stop(h, stop_mode, flags);
}

unsigned int vzctl_get_flags(void)
{
	return vzctl2_get_flags();
}

int vzctl_env_chkpnt(ctid_t ctid, int cmd,
		char *dumpfile,
		unsigned ctx,
		unsigned int cpu_flags,
		int cpt_flags,
		int skip_arpdetect)
{
	int ret;
	struct vzctl_cpt_param cpt = {
		.dumpfile = dumpfile,
		.ctx = ctx,
		.cpu_flags = cpu_flags,
		.cmd = cmd,
		.flags = cpt_flags,
	};
	int flags = (skip_arpdetect ? VZCTL_SKIP_ARPDETECT : 0);
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_chkpnt(h, cmd, &cpt, flags);
}

int vzctl_env_restore(ctid_t ctid, int cmd,
		char *dumpfile,
		unsigned ctx,
		unsigned int cpu_flags,
		int cpt_flags,
		int flags)
{
	int ret;
	struct vzctl_cpt_param cpt = {
		.dumpfile = dumpfile,
		.ctx = ctx,
		.cpu_flags = cpu_flags,
		.cmd = cmd,
		.flags = cpt_flags,
	};

	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_restore(h, &cpt, flags);
}

int vzctl_env_exec(ctid_t ctid, int exec_mode,
		char *const argv[], char *const envp[], char *std_in, int timeout, int flags)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_exec(h, exec_mode, argv, envp, std_in, timeout, flags);
}

int vzctl_env_exec_fn2(ctid_t ctid, execFn fn, void *data,
		int timeout, int flags)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_exec_fn2(h, fn, data, timeout, flags);
}

int vzctl_env_exec_fn_async(ctid_t ctid, const char *root, execFn fn, void *data,
        int *data_fd, int timeout, int flags, int *err)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_exec_fn_async(h, fn, data, data_fd, timeout, flags, err);
}

int vzctl_env_enter(ctid_t ctid)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);

	if (h == NULL)
		return ret;

	return vzctl2_env_enter(h);
}

int vzctl_env_set_userpasswd(ctid_t ctid, const char *user, const char *passwd, int is_crypted)
{
	int ret;
	int flags = is_crypted ? VZCTL_SET_USERPASSWD_CRYPTED : 0;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_set_userpasswd(h, user, passwd, flags);
}

int vzctl_env_mount(ctid_t ctid, int skip_fsck)
{
	int ret;
	int flags = skip_fsck ? VZCTL_SKIP_FSCK : 0;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_mount(h, flags);
}

int vzctl_env_umount(ctid_t ctid)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_umount(h, 0);
}

int vzctl_set_cpu_param(ctid_t ctid,
		unsigned long *vcpus,
		unsigned long *weight,
		unsigned long *units,
		struct cpu_limit *limit,
		struct vzctl_cpumask *cpumask,
		struct vzctl_nodemask *nodemask)
{
	int ret;
	struct vzctl_env_handle *h;
	vzctl_env_param_ptr param;

	if (vcpus == NULL && weight == NULL && units == NULL &&
			limit == NULL && cpumask == NULL && nodemask == NULL)
		return 0;

	param = vzctl2_alloc_env_param();
	if (param == NULL)
		return VZCTL_E_NOMEM;

	h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		goto err;

	if (vcpus != NULL &&
			(ret = vzctl2_env_set_cpu_count(param, *vcpus)))
		goto err;

	if (limit != NULL) {
		struct vzctl_cpulimit_param cpulimit = {
			.limit = limit->limit,
			.type = limit->type
		};

		ret = vzctl2_env_set_cpulimit(param, &cpulimit);
		if (ret)
			goto err;
	}
	if (weight != NULL &&
			(ret = vzctl2_env_set_cpuunits(param, *units * 500000)))
		goto err;

	if (units != NULL &&
			(ret = vzctl2_env_set_cpuunits(param, *units)))
		goto err;

	if (nodemask != NULL || cpumask != NULL) {
		ret = vzctl2_env_set_nodemask_ex(param, cpumask, nodemask);
		if (ret)
			goto err;
	}

	ret = vzctl2_apply_param(h, param, 0);

err:
	vzctl2_free_env_param(param);

	return ret;
}

int vzctl_set_node_cpu_param(unsigned long units)
{
#if 0
	int ret;
	struct vzctl_env_handle *h;
	vzctl_env_param_ptr param;

	param = vzctl2_alloc_env_param();
	if (param == NULL)
		return VZCTL_E_NOMEM;

	h = vzctl2_alloc_env_handle();
	if (h == NULL)
		goto err;

	ret = vzctl2_env_set_cpuunits(param, units);
	if (ret)
		goto err;

	ret = vzctl2_apply_param(h, param, 0);

err:
	vzctl2_env_close(h);
	vzctl2_free_env_param(param);

	return ret;
#endif
	return 0;
}

int vzctl_env_convert_layout(ctid_t ctid, const char *ve_conf, int new_layout)
{
#if 0
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, ve_conf, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_convert_layout(h, new_layout);
#endif
	return 0;
}

char *hwaddr2str(const char *hwaddr)
{
	char *str;
#define STR2MAC(dev)                    \
	((unsigned char *)dev)[0],      \
	((unsigned char *)dev)[1],      \
	((unsigned char *)dev)[2],      \
	((unsigned char *)dev)[3],      \
	((unsigned char *)dev)[4],      \
	((unsigned char *)dev)[5]

	str = (char *)malloc(18);
	sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
			STR2MAC(hwaddr));

	return str;
}

int vzctl_env_set_rate(ctid_t ctid)
{
	int ret;
	struct vzctl_env_handle *h;

	h = vzctl_env_open(ctid, NULL, VZCTL_CONF_SKIP_PARSE, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_set_tc_param(h, get_env_param(ctid), 0);
}

int vzctl_apply_param(ctid_t ctid, int setmode)
{
	struct vzctl_env_handle *h;
	struct vzctl_env_param *param;
	int ret, flags = 0;

	h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	param = get_env_param(ctid);
	if (param == NULL)
		return VZCTL_E_NOMEM;

	if (_g_apply_conf != NULL) {
		/* merge conf + cmd */
		ret = vzctl2_merge_env_param(_g_apply_conf, param);
		if (ret)
			return ret;
		param = vzctl2_get_env_param(_g_apply_conf);
		flags |= VZCTL_APPLY_CONF;
	}

	if (setmode)
		vzctl2_env_set_setmode(param, setmode);

	return vzctl2_apply_param(h, param, flags);
}

int vzctl_env_destroy(ctid_t ctid)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);

	if (h == NULL)
		return ret;

	return vzctl2_env_destroy(h, 0);
}

static const char *gen_uuid(char *buf)
{
	uuid_t out;

	uuid_generate(out);
	uuid_unparse(out, buf);

	return buf;
}

static int vzctl_add_disk(ctid_t ctid, struct vzctl_disk_param *param, int flags)
{
	int ret;
	char path[PATH_MAX];
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	/* generate default disk name */
	if (param->path == NULL && param->storage_url == NULL) {
		char u[39] = "";

		snprintf(path, sizeof(path), "disk-%s.hdd",
			param->uuid[0] != '\0' ? param->uuid : gen_uuid(u));
		param->path = path;
	}

	return vzctl2_env_add_disk(h, param, flags);
}

int vzctl_set_disk(ctid_t ctid, struct vzctl_disk_param *param)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_set_disk(h, param);
}

int vzctl_encrypt_disk(ctid_t ctid, struct vzctl_disk_param *param, int flags)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_encrypt_disk(h, param->uuid, param->enc_keyid, flags);
}

int vzctl_del_disk(ctid_t ctid, const char *guid, int detach)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_del_disk(h, guid, detach ? VZCTL_DISK_DETACH: 0);
}

int vzctl_configure_disk(ctid_t ctid, int op, struct vzctl_disk_param *param,
		int recreate, int flags)
{
	if (op == DEVICE_ACTION_ADD)
		return vzctl_add_disk(ctid, param,
				recreate ? VZCTL_DISK_RECREATE : 0);
	else if (op == DEVICE_ACTION_DEL)
		return vzctl_del_disk(ctid, param->uuid, 0);
	else if (op == DEVICE_ACTION_DETACH)
		return vzctl_del_disk(ctid, param->uuid, 1);
	else if (op == DEVICE_ACTION_SET) {
		if (param->enc_keyid) {
			int ret = vzctl_encrypt_disk(ctid, param, flags);
			if (ret)
				return ret;
		}
		return vzctl_set_disk(ctid, param);
	}

	return VZCTL_E_INVAL;
}

int vzctl_env_is_run(ctid_t ctid)
{
	int ret;
	vzctl_env_status_t status = {};
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return -1;

	if (vzctl2_get_env_status_info(h, &status, ENV_STATUS_RUNNING))
		return -1;

	if (status.mask & ENV_STATUS_RUNNING)
		return 1;
	return 0;
}

int vzctl_add_env_param_by_name(ctid_t ctid, const char *name, const char *str)
{
	struct vzctl_env_param *param = get_env_param(ctid);

	if (param == NULL)
		return VZCTL_E_NOMEM;

	return vzctl2_add_env_param_by_name(param, name, str);
}

int vzctl_add_env_param_by_id(ctid_t ctid, int id, const char *str)
{
	struct vzctl_env_param *param = get_env_param(ctid);

	if (param == NULL)
		return VZCTL_E_NOMEM;

	return vzctl2_add_env_param_by_id(param, id, str);
}

int vzctl_env_save(ctid_t ctid)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_env_save(h);
}

int vzctl_set_name(ctid_t ctid, const char *name)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_set_name(h, name);
}

int vzctl_env_open_apply_conf(ctid_t ctid, const char *conf)
{
	int err;
	char fname[PATH_MAX];

	vzctl2_get_config_full_fname(conf, fname, sizeof(fname));

	_g_apply_conf = vzctl2_env_open_conf(ctid, fname,
			VZCTL_CONF_SKIP_GLOBAL | VZCTL_CONF_SKIP_PRIVATE, &err);
	if (_g_apply_conf == NULL)
		return err;

	return 0;
}

int vzctl_env_create(ctid_t ctid,
		char *uuid,
		char *config,
		char *ostmpl,
		char *ve_private,
		char *ve_root,
		char *name,
		char *enc_keyid,
		int no_hdd,
		int layout)
{
	int ret;
	struct vzctl_env_param *env;
	struct vzctl_env_create_param create_param = {
		.uuid = uuid,
		.config = config,
		.ostmpl = ostmpl,
		.ve_private = ve_private,
		.ve_root = ve_root,
		.name = name,
		.layout = layout,
		.no_root_disk = no_hdd,
		.enc_keyid = enc_keyid,
	};

	SET_CTID(create_param.ctid, ctid)

	env = get_env_param(ctid);

	ret = vzctl2_env_create(env, &create_param, 0);

	vzctl2_free_env_param(env);

	return ret;
}

int vzctl_env_reinstall(ctid_t ctid,
		int skipbackup,
		int resetpwdb,
		int skipscripts,
		char *reinstall_scripts,
		char *reinstall_opts,
		char *ostemplate)
{
	struct vzctl_reinstall_param param = {
		.skipbackup = skipbackup,
		.resetpwdb = resetpwdb,
		.skipscripts = skipscripts,
		.reinstall_scripts = reinstall_scripts,
		.reinstall_opts = reinstall_opts,
		.ostemplate = ostemplate,
	};
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	ret = vzctl2_env_reinstall(h, &param);

	return ret;
}

int vzctl_del_param_by_id(ctid_t ctid, int id)
{
	int ret;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	return vzctl2_del_param_by_id(h, id);
}

int vzctl_renew_veth_mac(ctid_t ctid, const char *ifname)
{
	int ret;
	vzctl_veth_dev_iterator i = NULL, itd;
	struct vzctl_env_handle *h = vzctl_env_open(ctid, NULL, 0, &ret);
	if (h == NULL)
		return ret;

	struct vzctl_env_param *env = get_env_param(ctid);
	if (env == NULL)
		return VZCTL_E_NOMEM;

	while ((i = vzctl2_env_get_veth(vzctl2_get_env_param(h), i))) {
		struct vzctl_veth_dev_param d = {};

		vzctl2_env_get_veth_param(i, &d, sizeof(d));
		if (ifname != NULL && strcmp(ifname, d.dev_name_ve))
			continue;

		struct vzctl_veth_dev_param dev = {
			.dev_name_ve = d.dev_name_ve,
			.mac_renew = 1,
			.dhcp = -1,
			.dhcp6 = -1,
		};
		itd = vzctl2_create_veth_dev(&dev, sizeof(dev));
		if (itd == NULL)
			return vzctl_err(VZCTL_E_INVAL, 0,
					"vzctl2_create_veth_dev");
		vzctl2_env_add_veth(env, itd);
	}

	return 0;
}
