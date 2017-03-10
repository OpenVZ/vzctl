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
 *
 * A header file for ubclogd.
 */

#ifndef _UBCLOGD_H_
#define _UBCLOGD_H_

#define PROGRAM_NAME	"ubclogd"
#define ULD_VERSION		"2.1.0"
#define ULD_RELEASE		"2"
#define FULL_NAME	PROGRAM_NAME " v." ULD_VERSION "-" ULD_RELEASE
#define STARTUP_MSG	FULL_NAME " started.\n"
#define UBC_HEADER "       uid  resource           held    maxheld" \
		   "    barrier      limit    failcnt\n"

#define UPDATE_INTERVAL	60
#define RESERVED_SPACE	512
#define BLOCKSIZE	1024
/* How many blocks can be used before expanding */
#define BLOCK_THRESHOLD	4
/* How many extra blocks to add, not counting used blocks */
#define BLOCK_EXTRA	4
#define LOG_FILE	"/var/log/ubc"
#define UBC_FILE	"/proc/user_beancounters"
#define FILL_CHAR	'\0'


/* List of UBC parameters we log */
char * ubc_to_log[] =
{
	"kmemsize",
	"numproc",
	"privvmpages",
	"physpages",
	"oomguarpages",
	NULL
};

#endif /* _UBCLOGD_H_ */
