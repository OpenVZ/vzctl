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

#ifndef	__VETH_H__
#define	__VETH_H__

#include "list.h"

#define IFNAMSIZE	16
#define ETH_ALEN	6
#define MAC_SIZE	3*ETH_ALEN - 1
#define VZNETCFG	"/usr/sbin/vznetcfg"

#define PROC_VETH	"/proc/vz/veth"

#define SW_OUI		0x001851
#define DEF_VETHNAME	"veth$VEID"

#define VZCTL_VETH_CONFIGURE_ALL       0x01
#define VZCTL_VETH_CONFIGURE_NONE      0x02

struct vzctl_ip_param {
	list_elem_t list;
	char *ip;
	unsigned int mask;
	int op;
};
typedef struct vzctl_ip_param vzctl_ip_param_t;
typedef struct vzctl_ip_param veth_ip;

/** Data structure for devices. */
typedef struct veth_dev {
	list_elem_t list;               /**< next element. */
	list_head_t ip;
	char mac[ETH_ALEN];             /**< device MAC address. */
	int addrlen;                    /**< device MAC address length. */
	char dev_name[IFNAMSIZE+1];     /**< device name. */
	char mac_ve[ETH_ALEN];          /**< device MAC address in VPS. */
	int addrlen_ve;                 /**< VPS device MAC address length. */
	char dev_name_ve[IFNAMSIZE+1];  /**< device name in VPS. */
	int flags;
	char *gw;                       /**< gateway ip */
	char *gw6;                      /**< gateway ip6 */
	char *network;                  /**< connect virtual interface to
					  virtual network.
					  */
	int dhcp;                       /**< DHCP on/off. */
	int dhcp6;                      /**< DHC6P on/off. */
	int active;
	int configure;                  /**< configure flag */
	int custom_dev_name;
	int mac_filter;
	int configure_mode;             /** configure network related
					  parameters mask
					  */
} veth_dev;

/** Devices list.
 *  */
typedef struct veth_param {
	list_head_t dev;
	int delall;
} veth_param;

#endif
