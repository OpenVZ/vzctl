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

/*
 * Traffic accouting management structures
 */

#ifndef __ENVID_T_DEFINED__
# define __ENVID_T_DEFINED__
typedef unsigned int envid_t;
#endif

struct vz_tc_class_info {
	__u32				cid;	/* class number */
	__u32				addr;	/* Network byte order */
	__u32				mask;	/* subnet mask */
	/*
	 * On any changes to this struct keep in mind fixing
	 * all copy_to_user instances, initializing new fields/paddings
	 * to prevent possible leaks from kernel-space.
	 */
};


struct vzctl_tc_classes {
	struct vz_tc_class_info		*info;
	int				length;
};

/* For IPv6 */
struct vz_tc_class_info_v6 {
	__u32				cid;	/* class number */
	__u32				addr[4];/* Network byte order */
	__u32				mask[4];/* subnet mask */
	/*
	 * On any changes to this struct keep in mind fixing
	 * all copy_to_user instances, initializing new fields/paddings
	 * to prevent possible leaks from kernel-space.
	 */
};

struct vzctl_tc_classes_v6 {
	struct vz_tc_class_info_v6	*info;
	int				length;
};

struct vzctl_tc_get_stat {
	envid_t				veid;
	__u64				*incoming;
	__u64				*outgoing;
	__u32				*incoming_pkt;
	__u32				*outgoing_pkt;
	int				length;
};

struct vzctl_tc_get_stat_list {
	envid_t				*list;
	int				length;
};

char *get_ipname(unsigned int ip);
int tc_max_class(void);
int tc_set_classes(struct vz_tc_class_info *c_info, int length);
int tc_set_v6_classes(struct vz_tc_class_info_v6 *c_info, int length);
int tc_get_class_num(void);
int tc_get_v6_class_num(void);
int tc_get_classes(struct vz_tc_class_info *c_info, int length);
int tc_get_v6_classes(struct vz_tc_class_info_v6 *info, int length);
int tc_get_stat(ctid_t ctid, struct vzctl_tc_get_stat *stat, int v6);
int tc_get_base(int veid);
int tc_destroy_ve_stats(ctid_t ctid);
int tc_destroy_all_stats(void);
int get_ve_list(struct vzctl_tc_get_stat_list *velist);

#endif
