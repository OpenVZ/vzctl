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

#ifndef _VZERROR_H_
#define _VZERROR_H_

/* vzctl error codes */

/*****************************
    System errors
******************************/
#define VZ_SETUBC_ERROR			1
#define VZ_SETFSHD_ERROR		2
#define VZ_SYSTEM_ERROR			3
#define VZCTL_E_SYSTEM			VZ_SYSTEM_ERROR
#define VZ_NOMEM_ERROR			4
#define VZCTL_E_NOMEM			VZ_NOMEM_ERROR

/* not a VZ-capable kernel */
#define VZ_BAD_KERNEL			5
#define VZ_RESOURCE_ERROR		6
#define VZ_ENVCREATE_ERROR		7

#define VZ_COMMAND_EXECUTION_ERROR	8
#define VZ_LOCKED			9
/* global config file not found */
#define VZ_NOCONFIG			10
#define VZ_NOSCRIPT			11
#define VZ_NO_LICENSE			12
#define VZ_SET_CAP			13
#define VZ_NOVE_CONFIG			14
#define VZ_EXEC_TIMEOUT			15
#define VZCTL_E_CHKPNT			16
#define VZ_RESTORE_ERROR		17
#define VZ_SETLUID_ERROR		18
#define VZCTL_E_SETLUID			VZ_SETLUID_ERROR
#define VZCTL_E_SCRIPT_EXECUTION	19

/****************************
    Argument errors
 ****************************/
#define VZ_INVALID_PARAMETER_SYNTAX	20
#define VZ_INVALID_PARAMETER_VALUE	21
#define VZCTL_E_INVAL			VZ_INVALID_PARAMETER_VALUE
#define VZ_VE_ROOT_NOTSET		22
#define VZ_VE_PRIVATE_NOTSET		23
#define VZ_VE_TMPL_NOTSET		24
#define VZ_RATE_NOTSET			25
#define VZ_TOTALRATE_NOTSET		26
#define VZ_NOT_ENOUGH_PARAMS		27
#define VZ_NOT_ENOUGH_UBC_PARAMS	28
#define VZ_VE_PKGSET_NOTSET		29
#define VZ_VE_BANDWIDTH_NOTSET		30
/*****************************
    VE errors
 *****************************/
#define VZ_VE_NOT_RUNNING		31
#define VZCTL_E_ENV_NOT_RUN		VZ_VE_NOT_RUNNING
#define VZ_VE_RUNNING			32
#define VZ_STOP_ERROR			33
#define VZCTL_E_ENV_STOP		VZ_STOP_ERROR
#define VZ_CANT_ADDIP			34
#define VZ_VALIDATE_ERROR		35
#define VZ_OVERCOMMIT_ERROR		36

/****************************
    Filesystem errros
 ****************************/
/* private area is not mounted */
#define VZ_FS_NOT_MOUNTED		40
/* private area is already mounted */
#define VZ_FS_MOUNTED			41
/* no private area with this id */
#define VZ_FS_NOPRVT			43
/* private area with this id already exists */
#define VZ_FS_PRVT_AREA_EXIST		44
#define VZ_FS_NO_DISK_SPACE		46
/* template private area is not properly created */
#define VZ_FS_BAD_TMPL			47
/* cannot create new private area */
#define VZ_FS_NEW_VE_PRVT		48
/*  cannot create mounpoint */
#define VZ_FS_MPOINTCREAT		49
/* cannot mount ve private area */
#define VZ_FS_CANTMOUNT			50
/* cannot umount ve private area */
#define VZ_FS_CANTUMOUNT		51
/*  error deleting ve private area */
#define VZ_FS_DEL_PRVT			52
/* private area doesn't exist */
#define VZ_UNK_MOUNT_TYPE		53
#define VZ_CANT_CREATE_DIR		54
#define VZ_NOTENOUGH_QUOTA_LIMITS	55
/* Unsupported /sbin/init in private */
#define VZ_FS_BAD_SBIN_INIT		56
/*********************************
   Disk quota errors
 *********************************/
/* disk quota not supported */
#define VZ_DQ_ON			60
#define VZ_DQ_INIT			61
#define VZ_DQ_SET			62
#define VZ_DISKSPACE_NOT_SET		63
#define VZ_DISKINODES_NOT_SET		64
#define VZ_ERROR_SET_USER_QUOTA		65
#define VZ_DQ_OFF			66
#define VZ_DQ_UGID_NOTINITIALIZED	67
#define VZ_GET_QUOTA_USAGE		68
/* add some more codes here */

/*********************************
   "vzctl set" errors
 *********************************/
/* incorrect hostname */
#define VZ_BADHOSTNAME			70
/* incorrect ip address */
#define VZ_BADIP			71
/* incorrect dns nameserver address */
#define VZ_BADDNSSRV			72
/* incorrect dns domain name */
#define VZ_BADDNSSEARCH			73
/* error changing password */
#define VZ_CHANGEPASS			74
#define VZ_CLASSID_NOTSET		76
#define VZ_VE_LCKDIR_NOTSET		77
#define VZ_IP_INUSE			78
#define VZ_ACTIONSCRIPT_ERROR		79
#define VZ_SET_RATE			80
#define VZ_SET_ACCOUNT			81
#define VZ_CP_CONFIG			82
#define VZ_NOT_SHARED_BASE_VE		83
#define VZ_SHARED_BASE_VE_HAS_CHILD	84
#define VZ_INVALID_CONFIG		85
#define VZ_SET_DEVICES			86
#define VZ_INSTALL_APPS_ERROR		87
#define VZ_START_SHARED_BASE		88
#define VZ_INCOMPATIBLE_LAYOUT		89

/* Template Error */
#define VZ_PKGSET_NOT_FOUND		91
/* Recover	*/
#define VZ_RECOVER_ERROR		92
#define VZ_GET_APPS_ERROR		93
#define VZ_REINSTALL_ERROR		94
#define VZ_APP_CONFIG_ERROR		100
#define VZ_CUSTOM_CONFIGURE_ERROR	102
//#define VZ_IPREDIRECT_ERROR		103
#define VZ_NETDEV_ERROR			104
#define VZ_VE_START_DISABLED		105
#define VZ_SET_IPTABLES			106

#define VZ_NO_DISTR_CONF		107
#define	VZ_NO_DISTR_ACTION_SCRIPT	108

#define VZ_CUSTOM_REINSTALL_ERROR	128
#define VZ_SLMLIMIT_NOT_SET		129
#define VZ_SLM				130
#define VZ_UNSUP_TECH			131
#define VZ_SLM_DISABLED			132
#define VZ_WAIT_FAILED			133
#define VZ_SET_PERSONALITY		134
#define VZ_SET_MEMINFO_ERROR		135
#define VZ_VETH_ERROR			136
#define VZ_SET_NAME_ERROR		137
#define VZ_NO_INITTAB			138
#define VZ_CONF_SAVE_ERROR		139
#define VZ_REGISTER_ERROR		140
#define	VZ_VE_MANAGE_DISABLED		141
#define VZ_UNREGISTER_ERROR		142
#define VZ_UNSUPPORTED_ACTION		143
#define VZ_SET_OSRELEASE		144
#define VZ_GET_OSRELEASE		145
#define VZ_SET_CPUMASK			146
#define VZ_SET_PCI			147
#define VZ_SET_IO			148
#define VZ_SET_NODEMASK			149
#define VZ_NODE_TO_CPU			150

#define VZCTL_E_CREATE_IMAGE		151
#define VZCTL_E_MOUNT_IMAGE		152
#define VZCTL_E_UMOUNT_IMAGE		153
#define VZCTL_E_RESIZE_IMAGE		154
#define VZCTL_E_CONVERT_IMAGE		155
#define VZCTL_E_CREATE_SNAPSHOT		156
#define VZCTL_E_MERGE_SNAPSHOT		157
#define VZCTL_E_DELETE_SNAPSHOT		158
#define VZCTL_E_SWITCH_SNAPSHOT		159
#define VZCTL_E_MOUNT_SNAPSHOT		160
#define VZCTL_E_UMOUNT_SNAPSHOT		161
#define VZCTL_E_PARSE_DD		162
#define VZCTL_E_ADD_IMAGE		163
#define VZCTL_E_DEL_IMAGE		164
#define VZCTL_E_DISK_CONFIGURE		165
#define VZCTL_E_COMPACT_IMAGE		166
#define VZCTL_E_LIST_SNAPSHOT		167

#define VZCTL_E_PIPE			200
#define VZCTL_E_FORK			201

#define VZCTL_E_HAMAN			202
#ifdef __cplusplus
extern "C" {
#endif

/** Return test message of the last error
 */
const char *vzctl_get_last_error(void);
#ifdef __cplusplus
}
#endif
#endif /* _VZ_ERROR_H_ */
