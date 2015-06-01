/* Copyright (C) SWsoft, 1999-2007. All rights reserved.*/
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
