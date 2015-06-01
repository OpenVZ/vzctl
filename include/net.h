/*
 *
 *  Copyright (C) SWsoft, 1999-2007. All rights reserved.
 *
 */

/* $Id$ */
#ifndef _NET_H_
#define _NET_H_

struct CList;

int ext_ip_setup(int veid, struct CParam *param, int starting);
struct CList *get_vps_ip(int veid);
int vzctl_ip_ctl(unsigned veid, int op, char *ipstr);
#endif
