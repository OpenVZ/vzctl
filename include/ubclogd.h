/* $Id$
 *
 * Copyright (C) SWsoft, 1999-2007. All rights reserved.
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
