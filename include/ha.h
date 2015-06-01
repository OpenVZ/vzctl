/*
 *  Copyright 2012 Parallels IP Holdings GmbH. All Rights Reserved.
 */
#ifndef	_HA_H_
#define	_HA_H_

struct ha_params {
	int ha_enable;
	unsigned long *ha_prio;
};

int handle_set_cmd_on_ha_cluster(int veid, const char *ve_private,
		struct ha_params *cmdline,
		struct ha_params *config,
		int save);

#endif	/* _HA_H_ */
