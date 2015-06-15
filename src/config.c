/*
 *
 *  Copyright (C) SWsoft, 1999-2007. All rights reserved.
 *
 */

/* $Id$ */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/mount.h>
#include <langinfo.h>
#include <locale.h>
#include <limits.h>
#include <linux/vzlist.h>
#include <sys/sysmacros.h>

#include "vzctl.h"
#include "config.h"
#include "vzerror.h"
#include "tmplmn.h"
#include "util.h"
#include "cpu.h"

int vzctl_unescapestr_eq(char *src, char *dst, int size);

long _PAGE_SIZE = 4096;
struct Cconfig Config[] = {
{"EXT_IP_ADDRESS", NULL, &tmpparam.ext_ipadd, NULL, PARAM_EXT_IP_ADD, IP_ADDR, 0, 0},
{"", NULL, &tmpparam.ext_ipdel, NULL, PARAM_EXT_IP_DEL, IP_ADDR, 0, 0},
{"", NULL, &tmpparam.ipdelall, NULL, -1, FLAG, 0, 0},
{"", NULL, &tmpparam.skip_arpdetect, NULL, PARAM_SKIP_ARPDETECT, FLAG, 0, 0},
/*	Capability	*/
{"CONFIGFILE", NULL, &tmpparam.config, NULL, PARAM_CONFIG, STR, 0, 0},
/*	Fairshed	*/
{"BURST_CPU_AVG_USAGE", NULL, &tmpparam.br_cpu_avg_usage, NULL, PARAM_BR_CPU_AVG_USAGE, ULONG, 0, 0},
{"BURST_CPULIMIT", NULL, &tmpparam.br_cpulimit, NULL, PARAM_BR_CPULIMIT, ULONG, 0, 0},
/*	*/
{"NOATIME", NULL, &tmpparam.noatime, NULL, PARAM_NOATIME, YESNO, 0, 0},
/* Paths*/
{"APPLY_IPONLY", NULL, &tmpparam.apply_iponly, NULL, PARAM_APPLY_IPONLY, YESNO, 0, 0},
{"", NULL, &tmpparam.save, NULL, PARAM_SAVE, FLAG, 0, 0},
{"", NULL, &tmpparam.applyconfig, NULL, PARAM_APPCONF, STR, 0, 0},
{"", NULL, &tmpparam.applyconfig_map, NULL, PARAM_APPCONF_MAP, APPCONF_MAP, 0, 0},
/* Template */
{"", NULL, &tmpparam.ostmpl, NULL, PARAM_OSTEMPLATE, STR, 0, 0},
/* Passwd */
{"", NULL, &tmpparam.userpw, NULL, PARAM_USERPW, LIST_PW, 0, 0},
{"", NULL, &tmpparam.is_crypted, NULL, PARAM_PW_CRYPTED, FLAG, 0, 0},
/* Shared */
{"VE_TYPE", NULL, &tmpparam.vetype, NULL, PARAM_VETYPE, VETYPE, 0, 0},
/* Devices */
{"VE_VALIDATE_ACTION", NULL, &tmpparam.validatemode, NULL, -1, STR, 0, 0},
{"VE_ENVIRONMENT", NULL, &tmpparam.ve_env, NULL, -1, LIST, 0, 0},
{"DISABLED", NULL, &tmpparam.start_disabled, NULL, PARAM_DISABLED, YESNO, 0, 0},
{"", NULL, &tmpparam.setmode, NULL, PARAM_SETMODE, SETMODE, 0, 0},
{"", NULL, &tmpparam.resetub, NULL, PARAM_RESETUB, FLAG, 0, 0},
{"", NULL, &tmpparam.netif_mac_renew, NULL, PARAM_NETIF_MAC_RENEW, FLAG, 0, 0},

{"OSRELEASE", NULL, &tmpparam.osrelease, NULL, PARAM_OSRELEASE, STR, 0, 0},
{"NAME", NULL, &tmpparam.name, NULL, PARAM_NAME, STR, 0, 0},

{NULL, NULL, NULL, NULL, -1, -1, 0}
};

struct CList* GetAliases(char *name);

static struct CSetMode SetMode[] =
	{ {"ignore", SET_IGNORE}, {"restart", SET_RESTART}};

static int ParseSetMode(const char *str)
{
	int i;

	if (str == NULL)
		return -1;
	for (i = 0; i < sizeof(SetMode) / sizeof(*SetMode); i++)
		if (!strcmp(SetMode[i].name, str))
			return SetMode[i].mode;
	return -1;
}

static int ParseULong(const char *str, unsigned long *val)
{
	char *tail;

	if (!str || !val || *str == '\0')
		return 1;

	if (!strcmp(str, "unlimited")) {
		*val = LONG_MAX;
		return 0;
	}
	errno = 0;
	*val = strtoul(str, (char **)&tail, 10);
	if (*tail != '\0' || errno == ERANGE)
		return 1;

	return 0;
}

/* This function parse string in form xxx:yyy
 * If :yyy is omitted, it is set to xxx.
 */
static int _ParseTwoLongs(const char *str, unsigned long *val)
{
	char *tail;
	unsigned long long tmp;
	int ret = 0;

	if (!str || !val || *str == '\0')
		return 1;
	errno = 0;
	tmp = strtoull(str, &tail, 10);
	if (errno == ERANGE)
		return 1;
	if (!strncmp(str, "unlimited", 9)) {
		tmp = LONG_MAX;
		tail = (char *)str + 9;
	} else if (tmp > LONG_MAX) {
		tmp = LONG_MAX;
		ret = ERR_LONG_TRUNC;
	}
	val[0] = tmp;
	if (*tail == ':')
	{
		tail++;
		errno = 0;
		tmp = strtoull(tail, &tail, 10);
		if (!strcmp(tail, "unlimited"))
			tmp = LONG_MAX;
		else if ((*tail != '\0') || (errno == ERANGE))
			return 1;
		if (tmp > LONG_MAX) {
			tmp = LONG_MAX;
			ret = ERR_LONG_TRUNC;
		}
		val[1] = tmp;
	}
	else if (*tail == 0)
	{
		val[1] = val[0];
	}
	else
	{
		return 1;
	}
	return ret;
}

/* This function parse string in form xxx[GMKPB]:yyy[GMKPB]
 * If :yyy is omitted, it is set to xxx.
 */
static int _ParseTwoLongsN(const char *str, unsigned long *val,
	int div, int def_div)
{
	char *tail;
	unsigned long long tmp, n = 0;
	int ret = 0;

	if (!str || !val || *str == '\0')
		return 1;

	errno = 0;
	tmp = strtoull(str, &tail, 10);
	if (errno == ERANGE)
		return 1;
	if (!strncmp(str, "unlimited", 9)) {
		tmp = LONG_MAX;
		tail = (char *)str + 9;
	} else if (str == tail) {
		return 1;
	} else if (*tail != ':' && *tail != '\0') {
		if (get_mul(*tail, &n) == 0)
			tmp = tmp * n / div;
		else
			return 1;
		tail++;
	} else {
		tmp /= def_div;
	}
	if (tmp > LONG_MAX) {
		tmp = LONG_MAX;
		ret = ERR_LONG_TRUNC;
	}
	val[0] = tmp;
	if (*tail == ':')
	{
		str = ++tail;
		errno = 0;
		tmp = strtoull(str, &tail, 10);
		if (errno == ERANGE)
			return 1;
		if (!strcmp(str, "unlimited")) {
			tmp = LONG_MAX;
		} else if (str == tail) {
			return 1;
		} else if (*tail != '\0') {
			if (get_mul(*tail, &n) == 0)
				tmp = tmp * n / div;
			else
				return 1;
		} else {
			tmp /= def_div;
		}
		if (tmp > LONG_MAX) {
			tmp = LONG_MAX;
			ret = ERR_LONG_TRUNC;
		}
		val[1] = tmp;
	}
	else if (*tail == '\0')
	{
		val[1] = val[0];
	}
	else
	{
		return 1;
	}
	return ret;
}

int ParseTwoLongs(const char *str, unsigned long *val, int type)
{
	int ret;

	switch (type) {
	case TWOULONG	:
		ret = _ParseTwoLongs(str, val);
		break;
	case TWOULONG_P :
		ret = _ParseTwoLongsN(str, val, _PAGE_SIZE, 1);
		break;
	case TWOULONG_N	:
		ret = _ParseTwoLongsN(str, val, 1, 1);
		break;
	case TWOULONG_K	:
		ret = _ParseTwoLongsN(str, val, 1024, 1);
		break;
	case TWOULONG_B2P :
		ret = _ParseTwoLongsN(str, val, _PAGE_SIZE, _PAGE_SIZE);
		break;
	default:
		ret = 1;
		break;
	}
	return ret;
}

static inline void free_veth_ip(list_head_t *head)
{
	veth_ip *tmp, *veth_t;

	list_for_each_safe(veth_t, tmp, head, list) {
		list_del(&veth_t->list);
		free(veth_t);
	}
	list_head_init(head);
}

static void free_veth_dev(veth_dev *dev)
{
	free(dev->network);
	free_veth_ip(&dev->ip);
}

int add_veth_ip(list_head_t *list, veth_ip *ip, int op)
{
	veth_ip *ip_t;

	ip_t = xmalloc(sizeof(*ip_t));
	if (ip_t == NULL)
		return -1;
	ip_t->ip = ip->ip != NULL ? strdup(ip->ip) : NULL;
	ip_t->mask = ip->mask;
	ip_t->op = op;

	list_add_tail(&ip_t->list, list);

	return 0;
}

int add_veth_param(veth_param *veth, veth_dev *dev)
{
	veth_dev *dev_t;
	veth_ip *ip;

	if (list_is_init(&veth->dev))
		list_head_init(&veth->dev);
	dev_t = xmalloc(sizeof(*dev_t));
	if (dev_t == NULL)
		return -1;
	memcpy(dev_t, dev, sizeof(*dev_t));
	if (dev->network != NULL)
		dev_t->network = strdup(dev->network);
	if (dev->gw != NULL)
		dev_t->gw = strdup(dev->gw);
	if (dev->gw6 != NULL)
		dev_t->gw6 = strdup(dev->gw6);
	if (dev->configure_mode)
		dev_t->configure_mode = dev->configure_mode;
	list_head_init(&dev_t->ip);
	list_for_each(ip, &dev->ip, list) {
		if (add_veth_ip(&dev_t->ip, ip, ip->op)) {
			free_veth_dev(dev_t);
			free(dev_t);
			return -1;
		}
	}
	list_add_tail(&dev_t->list, &veth->dev);

	return 0;
}

int get_ip_str(struct CList *ip, char *buf, int len)
{
	unsigned int mask = 0;
	char *sp, *ep;
	int r;

	if (ip->str == NULL)
		return -1;
	sp = buf;
	ep = buf + len;
	r = snprintf(sp, ep - sp, "%s", ip->str);
	if (r < 0 || sp + r >= ep)
		return -1;
	sp += r;
	if (ip->val1 != NULL &&
		(mask = *ip->val1) != 0)
	{
		int family;

		family = get_net_family(ip->str);
		if (family == AF_INET) {
			r = snprintf(sp, ep - sp, "/%s",
					get_ipname(mask));
		} else {
			r = snprintf(sp, ep - sp, "/%d",
					mask);

		}
		sp += r;
		if (r < 0 || sp + r >= ep)
			return -1;
	}
	return 0;
}

#define DELPARAM(name) \
if ((pcfile = ListFindVar(cfile, name, '='))) { \
	free(pcfile->str); \
	pcfile->str = NULL; \
	ret++; \
}

int FindName(char *name)
{
	int i;

	if (name == NULL)
		return -1;
	for (i = 0; Config[i].name != NULL; i++)
	{
		int ret;
		ret = Config[i].opt_srch ? STRNCOMP(name, Config[i].name) :
			strcmp(name, Config[i].name);
		if (ret)
			continue;
		if (Config[i].alias != NULL)
			return FindName(Config[i].alias);
		return i;
	}
	return -1;
}

int FindOption(int id)
{
	int i;

	for (i = 0; Config[i].name != NULL; i++)
	{
		if (Config[i].option_id != id)
			continue;
		if (Config[i].alias != NULL)
			return FindName(Config[i].alias);
		return i;
	}
	return -1;
}

struct CList* GetAliases(char *name)
{
	int i;
	struct CList *list = NULL;

	if (name == NULL)
		return NULL;
	for (i = 0; Config[i].name != NULL; i++)
	{
		if (Config[i].alias == NULL)
			continue;
		if (!strcmp(name, Config[i].alias))
			list = ListAddElem(list, Config[i].name, NULL, NULL);
	}
	return list;
}

int yesno_to_num(char *str)
{
	if (str == NULL)
		return 0;
	else if (!strcmp(str, "yes"))
		return YES;
	else if (!strcmp(str, "no"))
		return NO;
	else
		return 0;
}

int SetParam(int i, char *sp, int checkdup, int version, int unset)
{
	char *rtoken;
	int ret;

	switch (Config[i].type)
	{
	case TWOULONG	:
	case TWOULONG_P :
	case TWOULONG_N	:
	case TWOULONG_K	:
	case TWOULONG_B2P :
	{
		unsigned long  **p = (unsigned long **)Config[i].pvar;
		if (checkdup && *p != NULL)
			return ERR_DUP;
		/* If we have versioned files, do not accept parameters
		 * belonging to other version
		 */
		if (version != 0  &&
			Config[i].version != 0 &&
			Config[i].version != version)
		{
			return ERR_UNK;
		}
		*p = (unsigned long *)xcalloc(1, sizeof(unsigned long) * 2);
		if (unset) {
			break;
		}
		ret = ParseTwoLongs(sp, *p, Config[i].type);
		if (ret == ERR_LONG_TRUNC)
			return ret;
		if (ret)
		{
			free(*p); *p = NULL;
			return ERR_INVAL;
		}
		break;
	}
	case ULONG	:
	{
		unsigned long **p = (unsigned long**)Config[i].pvar;
		if (checkdup && *p != NULL)
			return ERR_DUP;
		*p = (unsigned long *)xcalloc(1, sizeof(unsigned long));
		if (unset)
			break;
		if (ParseULong(sp, *p))
		{
			free(*p); *p = NULL;
			return ERR_INVAL;
		}
		break;
	}
	case FLOAT	:
	{
		float **p = (float**)Config[i].pvar;
		if (checkdup && *p != NULL)
			return ERR_DUP;
		*p = (float *)xcalloc(1, sizeof(float));
		if (unset)
			break;
		if (sscanf(sp, "%f", *p) != 1)
		{
			free(*p); *p = NULL;
			return ERR_INVAL;
		}
		break;
	}
	case INT	:
	{
		int *p = (int*)Config[i].pvar;
		if (unset)
			break;
		if (parse_int(sp, p))
			return ERR_INVAL;
		break;
	}
	case IP_ADDR	:
	{
		struct CList **p = (struct CList**)Config[i].pvar;
		unsigned long *mask;
		char *ip_str = NULL;

		if (checkdup && *p != NULL)
			return ERR_DUP;
		if (unset) {
			*p = ListAddElem(*p, NULL, NULL, NULL);
			break;
		}
		if ((rtoken = strtok(sp, "\t ")) == NULL)
			return 0;
		do {
			/* Hack for 'all' magic word in --ipdel */
			if (!strcmp(rtoken, "all")) {
				if (Config[i].option_id == PARAM_EXT_IP_DEL)
					tmpparam.ext_ipdelall = 1;
				continue;
			}
			if (!strcmp(rtoken, "0.0.0.0") ||
			    !strcmp(rtoken, "::") ||
			    !strcmp(rtoken, "::0"))
			{
				continue;
			}
			mask = malloc(sizeof(*mask));
			if (parse_ip(rtoken, &ip_str, (unsigned int *)mask))
				return ERR_INVAL;
			if (ListFind(*p, ip_str) != NULL)
				continue;
			*p = ListAddElem(*p, ip_str, mask, NULL);
			free(ip_str);
		} while ((rtoken = strtok(NULL, " ")));
		break;
	}
	case LIST	:
	{
		struct CList **p = (struct CList**)Config[i].pvar;
		if (checkdup && *p != NULL)
			return ERR_DUP;
		if (unset || *sp == 0) {
			*p = ListAddElem(*p, "", NULL, NULL);
			return 0;
		}
		if ((rtoken = strtok(sp, "\t ")) == NULL)
			return 0;
		do {
			if (!ListFind(*p, rtoken))
				*p = ListAddElem(*p, rtoken, NULL, NULL);
		} while ((rtoken = strtok(NULL, " ")));
		break;
	}
	case LIST_PW	:
	{
		struct CList **p = (struct CList**)Config[i].pvar;
		if (checkdup && *p != NULL)
			return ERR_DUP;
		if (!ListFind(*p, sp))
			*p = ListAddElem(*p, sp, NULL, NULL);
		/* hide password #PSBM-12907 */
		memset(sp, 0, strlen(sp));
		break;
	}
	case APPCONF_MAP:
	{
		int *p = (int*)Config[i].pvar;

		if (checkdup && *p)
			return ERR_DUP;
		if (!strcmp("name", sp))
			*p = APPCONF_NAME;
		else
			return ERR_INVAL;
		break;
	}
	case STR	:
	{
		char **p = (char**)Config[i].pvar;
		if (checkdup && *p != NULL)
			return ERR_DUP;
		*p = strdup(sp);
		break;
	}
	case STR_GUID	:
	{
		char **p = (char**)Config[i].pvar;
		char guid[64];

		if (checkdup && *p != NULL)
			return ERR_DUP;
		if (unset)
			break;
		if (vzctl_get_normalized_guid(sp, guid, sizeof(guid)))
			return ERR_INVAL;
		/* remove {} */
		guid[37] = '\0';
		*p = strdup(guid + 1);
		break;
	}
	case STR_ESC	:
	{
		char **p = (char**)Config[i].pvar;
		char *buf;
		int len;

		if (checkdup && *p != NULL)
			return ERR_DUP;
		len = strlen(sp) * 3;
		buf = xmalloc(len + 1);
		vzctl_unescapestr_eq(sp, buf, len);
		*p = buf;
		break;
	}
	case STR_UTF8	:
	{
		char **p = (char**)Config[i].pvar;
		int len;

		if (checkdup && *p != NULL)
			return ERR_DUP;
		len = strlen(sp) * 2 + 2;
		*p = xmalloc(len);
		if (vzctl_convertstr(sp, *p, len)) {
			free(*p);
			return ERR_INVAL;
		}
		break;
	}
	case STR_NAME	:
	{
		char **p = (char**)Config[i].pvar;
		int len;

		if (checkdup && *p != NULL)
			return ERR_DUP;
		if (sp[0] != 0 && !vzctl_is_env_name_valid(sp))
			return ERR_INVAL;
		len = strlen(sp) * 2 + 2;
		*p = xmalloc(len);
		if (vzctl_convertstr(sp, *p, len)) {
			free(*p); *p = NULL;
			return ERR_INVAL;
		}
		break;
	}
	case YESNO	:
	{
		int *p = (int *)Config[i].pvar;
		int n;

		if (unset) {
			*p = 0xff;
			break;
		}
		n = yesno2id(sp);
		if (n == -1)
			return ERR_INVAL;
		*p = n;
		break;
	}
	case ONOFF	:
	{
		int *p = (int *)Config[i].pvar;
		int n;

		if (unset) {
			*p = 0xff;
			break;
		}
		n = yesno2id(sp);
		if (n == -1)
			return ERR_INVAL;
		*p = n;
		break;
	}
	case FLAG	:
	{
		int *p = (int *)Config[i].pvar;
		*p = 1;
		break;
	}
	case VETYPE	:
	{
		int *p = (int *)Config[i].pvar;

		if (unset) {
			*p = 0xff;
			break;
		}

		if (checkdup && *p != 0)
			return ERR_DUP;

		if (!strcmp(sp, VE_TYPE_TEMPLATE))
			*p = VZCTL_ENV_TYPE_TEMPLATE;
		else if (!strcmp(sp, VE_TYPE_REGULAR))
			*p = VZCTL_ENV_TYPE_REGULAR;

		break;
	}
	case SETMODE	:
	{
		int *p = (int*)Config[i].pvar;
		int mode;
		if ((mode = ParseSetMode(sp)) < 0)
			return ERR_INVAL;
		*p = mode;
		break;
	}
	default	:
		break;
	}
	return 0;
}

void FreeParam(struct CParam *param)
{
	int i = 0;

	if (param == NULL)
		return;

	memcpy(&tmpparam, param, sizeof(struct CParam));
	while(Config[i++].name != NULL)
	{
		switch (Config[i].type)
		{
		case TWOULONG	:
		case TWOULONG_P	:
		case TWOULONG_N	:
		case TWOULONG_K	:
		case TWOULONG_B2P:
		case ULONG	:
		case FLOAT	:
		{
			float  **p = (float **)Config[i].pvar;
			free((float *)*p);
			*p = NULL;
			break;
		}
		case IP_ADDR	:
		case LIST	:
		case STR	:
		{
			char **p =  (char**)Config[i].pvar;

			free((char *) *p);
			*p = NULL;
			break;
		}
		default	:
			break;
		}
	}
	memset(param, 0, sizeof(struct CParam));
	return;
}

int get_swap(unsigned long long *swap)
{
	FILE *fd;
	char str[STR_SIZE];

	if ((fd = fopen(PROCMEM, "r")) == NULL)	{
		fprintf(stderr, "Cannot open " PROCMEM "\n");
		return 1;
	}
	while (fgets(str, sizeof(str), fd))
		if (sscanf(str, "SwapTotal: %llu", swap) == 1) {
			*swap *= 1024;
			fclose(fd);
			return 0;
		}

	fprintf(stderr, "Total swap value not found in " PROCMEM "\n");
	fclose(fd);
	return 1;
}

int get_lowmem(unsigned long long *mem)
{
	FILE *fd;
	char str[STR_SIZE];

	if ((fd = fopen(PROCMEM, "r")) == NULL)	{
		fprintf(stderr, "Cannot open " PROCMEM "\n");
		return 1;
	}
	*mem = 0;
	while (fgets(str, sizeof(str), fd)) {
		if (sscanf(str, "LowTotal: %llu", mem) == 1)
			break;
		/* Use MemTotal in case LowTotal not found */
		sscanf(str, "MemTotal: %llu", mem);
	}
	fclose(fd);
	if (*mem == 0) {
		fprintf(stderr, "Neither LowTotal nor MemTotal found in the "
			PROCMEM "\n");
		return 1;
	}
	*mem *= 1024;
	return 0;
}

int get_mem(unsigned long long *mem)
{
	if (mem == NULL)
		return 1;
	if ((*mem = sysconf(_SC_PHYS_PAGES)) == -1) {
		fprintf(stderr, "Unable to get the number of total physical pages\n");
		return 1;
	}
	*mem *= _PAGE_SIZE;
	return 0;
}

int get_pagesize(void)
{
	long pagesize;
	if ((pagesize = sysconf(_SC_PAGESIZE)) == -1) {
		fprintf(stderr, "Unable to get page size\n");
		return 1;
	}
	_PAGE_SIZE = pagesize;
	return 0;
}

int get_thrmax(int *thrmax)
{
	FILE *fd;
	char str[STR_SIZE];

	if (thrmax == NULL)
		return 1;
	if ((fd = fopen(PROCTHR, "r")) == NULL) {
		fprintf(stderr, "Cannot open " PROCTHR "\n");
		return 1;
	}
	if (fgets(str, sizeof(str), fd) == NULL) {
		fclose(fd);
		return 1;
	}
	fclose(fd);
	if (sscanf(str, "%du", thrmax) != 1)
		return 1;
	return 0;
}

int get_ubc(int veid, struct CParam *param)
{
	FILE *fd;
	char str[STR_SIZE];
	char name[STR_SIZE];
	const char *fmt;
	int res, found, id, i;
	unsigned long held, maxheld, barrier, limit;

	fd = fopen(PROCUBC, "r");
	if (fd == NULL) {
		fprintf(stderr, "Cannot open file %s\n", PROCUBC);
		return 1;
	}
	memset(&tmpparam, 0, sizeof(struct CParam));
	found = 0;
	while (fgets(str, STR_SIZE, fd)) {
		if ((res = sscanf(str, "%d:", &id)) == 1) {
			if (id == veid) {
				fmt =  "%*lu:%s%lu%lu%lu%lu";
				found = 1;
			} else {
				if (found)
					break;
			}
		} else {
			fmt = "%s%lu%lu%lu%lu";
		}
		if (found) {
			if ((res = sscanf(str, fmt,
				name, &held, &maxheld, &barrier, &limit)) == 5)
			{
				for (i = 0; name[i] != 0; i++)
					name[i] = toupper(name[i]);
				if ((i = FindName(name)) >= 0) {
					unsigned long **par = Config[i].pvar;
					*par = (unsigned long *)xmalloc
						(sizeof(unsigned long) * 2);
					(*par)[0] = held;
					(*par)[1] = held;
				}
			}
		}
	}
	memcpy(param, &tmpparam, sizeof(struct CParam));
	fclose(fd);
	return !found;
}

void *xmalloc(size_t size)
{
	void *p;

	p = malloc(size);
	if (p == NULL) {
		logger(-1, 0, "Unable to allocate %zd bytes", size);
		exit(VZ_RESOURCE_ERROR);
	}
	return p;
}

void *xcalloc(size_t nmemb, size_t size)
{
	void *p;

	p = calloc(nmemb, size);
	if (p == NULL) {
		logger(-1, 0, "Unable to allocate %zd bytes", size);
		exit(VZ_RESOURCE_ERROR);
	}
	return p;
}

static int s32_comp(const void *val1, const void *val2)
{
	return (*(int *) val1 - *(int *) val2);
}

int *find_veid(int veid, int *base, int nmemb)
{
	if (base == NULL || nmemb <= 0)
		return NULL;
	return bsearch(&veid, base, nmemb, sizeof(int), s32_comp);
}
