/* Copyright (C) SWsoft, 1999-2007. All rights reserved.*/
#ifndef	_DISTRCONF_H_
#define	_DISTRCONF_H_

enum {
	ADD_IP = 1,
	DEL_IP,
	SET_HOSTNAME,
	SET_DNS,
	SET_USERPASS,
	SET_UGID_QUOTA,
	SET_CONSOLE,
	POST_CREATE,
	NETIF_ADD,
	NETIF_DEL,
};

typedef struct {
	char *add_ip;
	char *del_ip;
	char *set_hostname;
	char *set_dns;
	char *set_userpass;
	char *set_ugid_quota;
	char *set_console;
	char *post_create;
	char *netif_add;
	char *netif_del;
} dist_actions;

struct CParam;
extern dist_actions *distActions;
int read_dist_actions(void);
int get_dist_action(char *name, char **script);
#endif
