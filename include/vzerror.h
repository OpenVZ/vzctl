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

#ifndef _ERROR_H_
#define _ERROR_H_

#include <vzctl/vzerror.h>

#define VZ_SYSTEM_ERROR			VZCTL_E_SYSTEM
#define VZ_NOMEM_ERROR			VZCTL_E_NOMEM
#define VZ_RESOURCE_ERROR		VZCTL_E_RESOURCE
#define VZ_COMMAND_EXECUTION_ERROR	VZCTL_E_EXEC
#define VZ_LOCKED			VZCTL_E_LOCKED
#define VZ_NOSCRIPT			VZCTL_E_NOSCRIPT
#define VZCTL_E_SCRIPT_EXECUTION	19
#define VZ_NOVE_CONFIG			VZCTL_E_NOVE_CONFIG
#define VZ_INVALID_PARAMETER_SYNTAX	VZCTL_E_INVAL_PARAMETER_SYNTAX
#define VZ_INVALID_PARAMETER_VALUE	VZCTL_E_INVAL
#define VZ_VE_ROOT_NOTSET		VZCTL_E_VE_ROOT_NOTSET
#define VZ_VE_PRIVATE_NOTSET		VZCTL_E_VE_PRIVATE_NOTSET
#define VZCTL_E_ENV_NOT_RUNG		VZCTL_E_ENV_NOT_RUN
#define VZ_FS_CANTMOUNT			VZCTL_E_MOUNT
#define VZ_FS_NOT_MOUNTED		VZCTL_E_FS_NOT_MOUNTED
#define VZ_VE_NOT_RUNNING		VZCTL_E_ENV_NOT_RUN
#define VZ_SET_NAME_ERROR		VZCTL_E_SET_NAME
#define VZ_VE_MANAGE_DISABLED		VZCTL_E_ENV_MANAGE_DISABLED
#define VZCTL_E_LIST_SNAPSHOT		167

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
