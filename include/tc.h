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

#ifndef _TC_H_
#define _TC_H_

#define VZCTLDEV	"/dev/vzctl"

#define ERR_BAD_IP		-1
#define ERR_TC_READ_CLASSES	-2
#define ERR_TC_NO_CLASSES	-3
#define ERR_IOCTL		-4
#define ERR_NOMEM		-5
#define ERR_INVAL_OPT		-6
#define ERR_TC_VZCTL		-7
#define ERR_OUT			-8
#define ERR_TC_INVAL_CLASS	-9
#define ERR_TC_INVAL_CMD	-10
#define ERR_READ_UUIDMAP	-11

char *get_ipname(unsigned int ip);
int tc_max_class(void);
int tc_set_classes(struct vz_tc_class_info *c_info, int length);
int tc_set_v6_classes(struct vz_tc_class_info_v6 *c_info, int length);
int tc_get_class_num(void);
int tc_get_v6_class_num(void);
int tc_get_classes(struct vz_tc_class_info *c_info, int length);
int tc_get_v6_classes(struct vz_tc_class_info_v6 *info, int length);
int tc_get_stat(struct vzctl_tc_get_stat *stat);
int tc_get_v6_stat(struct vzctl_tc_get_stat *stat);
int tc_get_base(int veid);
int tc_destroy_ve_stats(ctid_t ctid);
int tc_destroy_all_stats(void);
int get_ve_list(struct vzctl_tc_get_stat_list *velist);

#endif
