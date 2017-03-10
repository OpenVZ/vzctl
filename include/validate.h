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

#ifndef _VALIDATE_H_
#define _VALIDATE_H_

#define STR_ACT_NONE	"none"
#define STR_ACT_WARN	"warning"
#define STR_ACT_ERROR	"error"
#define STR_ACT_FIX	"fix"


#define ACT_NONE	0
#define ACT_WARN	1
#define ACT_ERROR	2
#define ACT_FIX		3

#define UTILIZATION	0
#define COMMITMENT	1

struct CRusage {
	double low_mem;
	double total_ram;
	double mem_swap;
	double alloc_mem;
	double alloc_mem_lim;
	double alloc_mem_max_lim;
	double cpu;
};

int validate(int veid, struct CParam *param, int recover, int ask);
int validate_ve(int veid, struct CParam *param);
struct CRusage *calc_hn_rusage(int mode);
struct CRusage * calc_ve_commitment(int veid, struct CParam *param,
	int numerator);
struct CRusage * calc_ve_utilization(struct CParam *param, int numerator);
int inc_rusage(struct CRusage *rusagetotal, struct CRusage *rusageve);
int check_hn_overcommitment(int veid, struct CParam *param);
int check_param(struct CParam *param, int log);
int mul_rusage(struct CRusage *rusage, int k);
#endif
