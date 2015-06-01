/* Copyright (C) SWsoft, 1999-2007. All rights reserved.*/
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
