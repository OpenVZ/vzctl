/*
 *  Copyright (C) SWsoft, 1999-2007. All rights reserved.
 *
 */
#ifndef _CPU_H_
#define _CPU_H_

#ifdef __cplusplus
extern "C" {
#endif
char *mask2str(unsigned long *mask, int size);
int parse_cpumask(const char *str, struct vzctl_cpumask **cpumask);
int parse_nodemask(const char *str, struct vzctl_nodemask **nodemask);

/** Get VE cpu statistics
*/
int vzctl_env_cpustat(unsigned veid, struct vzctl_cpustat *vzctl_cpustat,
	int size);

int vzctl_get_cpuinfo(struct vzctl_cpuinfo *info);

#ifdef __cplusplus
}
#endif

#endif /* _FS_H_ */
