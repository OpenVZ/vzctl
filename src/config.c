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
static char *hwid = NULL;
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
{"", NULL, &tmpparam.veth_add, NULL, PARAM_NETIF_ADD, NETIF_ADD_T, 0, 0},
{"", NULL, &tmpparam.veth_del, NULL, PARAM_NETIF_DEL, NETIF_DEL_T, 0, 0},
{"", NULL, &tmpparam.netif_mac_renew, NULL, PARAM_NETIF_MAC_RENEW, FLAG, 0, 0},
{"", NULL, &tmpparam.veth_add, NULL, PARAM_NETIF_DHCP, NETIF_DHCP, 0, 0},
{"", NULL, &tmpparam.veth_add, NULL, PARAM_NETIF_DHCP6, NETIF_DHCP6, 0, 0},
{"", NULL, &tmpparam.veth_add, NULL, PARAM_NETIF_NAMESERVER, NETIF_NAMESERVER, 0, 0},
{"", NULL, &tmpparam.veth_add, NULL, PARAM_NETIF_MAC, NETIF_MAC, 0, 0},
{"", NULL, &tmpparam.veth_add, NULL, PARAM_NETIF_NAME, NETIF_NAME, 0, 0},
{"", NULL, &tmpparam.veth_add, NULL, PARAM_NETIF_HOST_MAC, NETIF_HOST_MAC, 0, 0},
{"", NULL, &tmpparam.veth_add, NULL, PARAM_NETIF_HOST_IFNAME, NETIF_HOST_IFNAME, 0, 0},
{"", NULL, &tmpparam.veth_add, NULL, PARAM_NETIF_GW, NETIF_GW, 0, 0},
{"", NULL, &tmpparam.veth_add, NULL, PARAM_NETIF_GW6, NETIF_GW6, 0, 0},
{"", NULL, &tmpparam.veth_add, NULL, PARAM_NETIF_NETWORK, NETIF_NETWORK, 0, 0},
{"", NULL, &tmpparam.veth_add, NULL, PARAM_NETIF_MAC_FILTER, NETIF_MAC_FILTER, 0, 0},
{"", NULL, &tmpparam.veth_add, NULL, PARAM_NETIF_CONFIGURE_MODE, NETIF_CONFIGURE_MODE, 0, 0},

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

static void get_hwid(void)
{
	FILE *fp;
	int len;
	char mac[18] = "00:00:00:00:00:00";
	char buf[127];
	const char *cmd = "/sbin/ip a l | grep -m 1 'link/ether' 2>/dev/null";

	if (hwid != NULL)
		return;
	fp = popen(cmd, "r");
	if (fp == NULL)
		goto err;
	len = fread(buf, 1, sizeof(buf) - 1, fp);
	if (len > 0) {
		buf[len] = 0;
		sscanf(buf, "%*[^l]link/ether %17s", mac);
	}
	pclose(fp);
err:
	hwid = strdup(mac);
}

void generate_mac(int veid, char *dev_name, char *mac)
{
	int len, i;
	unsigned int hash, tmp;
	char data[128];

	get_hwid();
	snprintf(data, sizeof(data), "%s:%d:%s ", dev_name, veid, hwid);
	hash = veid;
	len = strlen(data) - 1;
	for (i = 0; i < len; i++) {
		hash += data[i];
		tmp = (data[i + 1] << 11) ^ hash;
		hash = (hash << 16) ^ tmp;
		hash += hash >> 11;
	}
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;
	mac[0] = (char) (SW_OUI >> 0xf);
	mac[1] = (char) (SW_OUI >> 0x8);
	mac[2] = (char) SW_OUI;
	mac[3] = (char) hash;
	mac[4] = (char) (hash >> 0x8);
	mac[5] = (char) (hash >> 0xf);
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

static inline void free_veth_ip(list_head_t *head)
{
	veth_ip *tmp, *veth_t;

	list_for_each_safe(veth_t, tmp, head, list) {
		list_del(&veth_t->list);
		free(veth_t);
	}
	list_head_init(head);
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

veth_dev *find_veth_by_ifname(list_head_t *head, char *name)
{
	veth_dev *dev_t;

	if (list_empty(head))
		return NULL;
	list_for_each(dev_t, head, list) {
		if (!strcmp(dev_t->dev_name, name))
			return dev_t;
	}
	return NULL;
}

veth_dev *find_veth_by_ifname_ve(list_head_t *head, char *name)
{
	veth_dev *dev_t;

	if (list_empty(head))
		return NULL;
	list_for_each(dev_t, head, list) {
		if (!strcmp(dev_t->dev_name_ve, name))
			return dev_t;
	}
	return NULL;
}

veth_dev *find_veth_configure(list_head_t *head)
{
	veth_dev *dev_t;

	if (list_empty(head))
		return NULL;
	list_for_each(dev_t, head, list) {
		if (dev_t->configure)
			return dev_t;
	}
	return NULL;
}

veth_ip *find_veth_ip(list_head_t *head, char *ip)
{
	veth_ip *ip_t;

	if (list_empty(head))
		return NULL;
	list_for_each(ip_t, head, list) {
		if (ip_t->ip != NULL && !strcmp(ip_t->ip, ip))
			return ip_t;
	}
	return NULL;
}

void fill_veth_dev(veth_dev *dst, veth_dev *src)
{
	if (src->dev_name[0] != 0)
		strcpy(dst->dev_name, src->dev_name);
	if (src->custom_dev_name)
		dst->custom_dev_name = src->custom_dev_name;
	if (src->addrlen != 0) {
		memcpy(dst->mac, src->mac, sizeof(dst->mac));
		dst->addrlen = src->addrlen;
	}
	if (src->dev_name_ve[0] != 0)
		strcpy(dst->dev_name_ve, src->dev_name_ve);
	if (src->addrlen_ve != 0) {
		memcpy(dst->mac_ve, src->mac_ve, sizeof(dst->mac));
		dst->addrlen_ve = src->addrlen_ve;
	}
	if (src->network != NULL) {
		free(dst->network);
		dst->network = strdup(src->network);
	}
	if (src->gw) {
		free(dst->gw);
		dst->gw = strdup(src->gw);
	}
	if (src->gw6) {
		free(dst->gw6);
		dst->gw6 = strdup(src->gw6);
	}

	if (src->dhcp)
		dst->dhcp = src->dhcp;
	if (src->dhcp6)
		dst->dhcp6 = src->dhcp6;

	if (src->mac_filter)
		dst->mac_filter = src->mac_filter;

	if (src->configure_mode)
		dst->configure_mode = src->configure_mode;
}

int merge_dev(veth_dev *old, veth_dev *new, veth_dev *merged)
{
	int ipdelall = 0;
	veth_ip *ip;

	memset(merged, 0, sizeof(veth_dev));
	list_head_init(&merged->ip);

	if (old != NULL) {
		/* process --ipdel all */
		list_for_each(ip, &new->ip, list) {
			if (ip->op == DELALL) {
				ipdelall = 1;
				break;
			}
		}
		if (!ipdelall) {
			list_for_each(ip, &old->ip, list) {
				if (find_veth_ip(&new->ip, ip->ip) != NULL)
					continue;
				add_veth_ip(&merged->ip, ip, ip->op);
			}
		}
		fill_veth_dev(merged, old);
		fill_veth_dev(merged, new);
	} else {
		fill_veth_dev(merged, new);
	}
	list_for_each(ip, &new->ip, list) {
		if (ip->op == ADD)
			add_veth_ip(&merged->ip, ip, ip->op);
	}
	return 0;
}

void free_veth_dev(veth_dev *dev)
{
	free(dev->network);
	free_veth_ip(&dev->ip);
}

void free_veth(list_head_t *head)
{
	veth_dev *tmp, *dev_t;

	if (list_empty(head))
		return;
	list_for_each_safe(dev_t, tmp, head, list) {
		free_veth_dev(dev_t);
		list_del(&dev_t->list);
		free(dev_t);
	}
	list_head_init(head);
}

void free_veth_param(veth_param *veth)
{
	free_veth(&veth->dev);
}

int copy_veth_param(veth_param *dst, veth_param *src)
{
	veth_dev *dev;

	list_for_each(dev, &src->dev, list) {
		if (add_veth_param(dst, dev))
			return 1;
	}
	return 0;
}

static int parse_netif_str_cmd(const char *str, veth_dev *dev)
{
	const char *ch, *tmp, *ep;
	int len, err;

	memset(dev, 0, sizeof(*dev));
	list_head_init(&dev->ip);
	ep = str + strlen(str);
	/* --netif_add <ifname[,mac,host_ifname,host_mac]]>] */
	/* Parsing veth device name in Container */
	if ((ch = strchr(str, ',')) == NULL) {
		ch = ep;
		len = ep - str;
	} else {
		len = ch - str;
		ch++;
	}
	if (len > IFNAMSIZE)
		return ERR_INVAL;
	snprintf(dev->dev_name_ve, len + 1, "%s", str);
	tmp = ch;
	if (ch == ep) {
		dev->addrlen_ve = ETH_ALEN;
		dev->addrlen = ETH_ALEN;
		return 0;
	}
	/* Parsing veth MAC address in Container */
	if ((ch = strchr(tmp, ',')) == NULL) {
		ch = ep;
		len = ch - tmp;
	} else {
		len = ch - tmp;
		ch++;
	}
	if (len != MAC_SIZE) {
		logger(-1, 0, "Invalid Container MAC address length");
		return ERR_INVAL;
	}
	dev->addrlen_ve = ETH_ALEN;
	err = parse_hwaddr(tmp, dev->mac_ve);
	if (err) {
		logger(-1, 0, "Invalid Container MAC address format");
		return ERR_INVAL;
	}
	dev->addrlen = ETH_ALEN;
	tmp = ch;
	if (ch == ep) {
		dev->addrlen = ETH_ALEN;
		return 0;
	}
	/* Parsing veth name in VE0 */
	if ((ch = strchr(tmp, ',')) == NULL) {
		ch = ep;
		len = ch - tmp;
	} else {
		len = ch - tmp;
		ch++;
	}
	if (len > IFNAMSIZE)
		return ERR_INVAL;
	snprintf(dev->dev_name, len + 1, "%s", tmp);
	dev->custom_dev_name = 1;
	if (ch == ep) {
		dev->addrlen = ETH_ALEN;
		return 0;
	}
	/* Parsing veth MAC address in VE0 */
	len = strlen(ch);
	if (len != MAC_SIZE) {
		logger(-1, 0, "Invalid server MAC address");
		return ERR_INVAL;
	}
	dev->addrlen = ETH_ALEN;
	err = parse_hwaddr(ch, dev->mac);
	if (err) {
		logger(-1, 0, "Invalid server MAC address");
		return ERR_INVAL;
	}
	return 0;
}

static int parse_netif_cmd(veth_param *param, char *val)
{
	char *token;
	veth_dev dev;

	if ((token = strtok(val, " ")) == NULL)
		return 0;
	do {
		if (parse_netif_str_cmd(token, &dev))
			return ERR_INVAL;
		add_veth_param(param, &dev);
	} while ((token = strtok(NULL, " ")));

	return 0;
}

static int add_netif_param(veth_param *veth, int opt, char *str)
{
	int len, ret;
	unsigned int mask;
	veth_dev *dev = NULL;
	int update_configure = 0;

	dev = find_veth_configure(&veth->dev);
	if (dev == NULL) {
		dev = xcalloc(1, sizeof(veth_dev));
		dev->configure = 1;
		list_head_init(&dev->ip);
		list_add_tail(&dev->list, &veth->dev);
	}
	len = strlen(str);
	switch (opt) {
	case NETIF_NAME:
		if (dev->dev_name_ve[0] != 0) {
			logger(-1, 0, "Specifying the --ifname option several times"
				" is not allowed");
			return ERR_INVAL;
		}
		if (len > IFNAMSIZE)
			return ERR_INVAL;
		strcpy(dev->dev_name_ve, str);
		break;
	case NETIF_MAC:
		if (parse_hwaddr(str, dev->mac_ve))
			return ERR_INVAL;
		dev->addrlen_ve = ETH_ALEN;
		break;
	case NETIF_HOST_IFNAME:
		if (len > IFNAMSIZE)
			return ERR_INVAL;
		strcpy(dev->dev_name, str);
		dev->custom_dev_name = 1;
		break;
	case NETIF_HOST_MAC:
		if (parse_hwaddr(str, dev->mac))
			return ERR_INVAL;
		dev->addrlen = ETH_ALEN;
		break;
	case NETIF_GW:
		if (str[0] == '\0') {
			dev->gw = strdup(str);
			break;
		}
		if (get_net_family(str) != AF_INET)
			return ERR_INVAL;
		if (parse_ip(str, &dev->gw, &mask))
			return ERR_INVAL;
		update_configure = 1;
		break;
	case NETIF_GW6:
		if (str[0] == '\0') {
			dev->gw6 = strdup(str);
			break;
		}
		if (get_net_family(str) != AF_INET6)
			return ERR_INVAL;
		if (parse_ip(str, &dev->gw6, &mask))
			return ERR_INVAL;
		update_configure = 1;
		break;
	case NETIF_DHCP:
		if (!(ret = yesno_to_num(str)))
			return ERR_INVAL;
		dev->dhcp = ret;
		update_configure  = 1;
		break;
	case NETIF_DHCP6:
		if (!(ret = yesno_to_num(str)))
			return ERR_INVAL;
		dev->dhcp6 = ret;
		update_configure = 1;
		break;
	case NETIF_NETWORK:
		if (str[0] != 0 && !vzctl_is_networkid_valid(str))
			return ERR_INVAL;
		if (dev->network == NULL)
			dev->network = strdup(str);
		break;
	case NETIF_MAC_FILTER:
		if (!strcmp(str, "on"))
			dev->mac_filter = YES;
		else if (!strcmp(str, "off"))
			dev->mac_filter = NO;
		else
			return ERR_INVAL;
		break;
	case NETIF_CONFIGURE_MODE:
		if (!strcmp(str, "none") ||
		    !strcmp(str, "no"))
			dev->configure_mode = VZCTL_VETH_CONFIGURE_NONE;
		else if (!strcmp(str, "all") ||
			 !strcmp(str, "yes"))
			dev->configure_mode = VZCTL_VETH_CONFIGURE_ALL;
		else
			return ERR_INVAL;
		break;
	}
	/* set VZCTL_VETH_CONFIGURE_ALL mode on parameters set */
	if (!dev->configure_mode && update_configure)
		 dev->configure_mode = VZCTL_VETH_CONFIGURE_ALL;
	return 0;
}

struct CList *ParseVariable(char *str)
{
	struct CList *l = NULL;
	char *tmp;
	char *ltoken, *rtoken, *ep, *sp;

	if (!str)
		return NULL;

	tmp = strdup(str);
	sp = tmp;
	ep = sp;
	while(*ep)
	{
		if (*ep == '"')
			*ep = ' ';
		ep++;
	}
	ep--;
	while ((isspace(*ep) || *ep == '"') && ep >= sp) *ep-- = '\0';

	ltoken = strtok(sp, "=");

	if (!ltoken || !(rtoken = strtok(NULL, "\t ")))
	{
		free(tmp);
		return NULL;
	}
	l = ListAddElem(l, rtoken, NULL, NULL);
	while ((rtoken = strtok(NULL, " ")))
		if (!ListFind(l, rtoken))
			l = ListAddElem(l, rtoken, NULL, NULL);
	free(tmp);
	return l;
}

struct CList *ParseIpVariable(char *str)
{
	struct CList *l = NULL;
	char *sp, *rtoken;
	char ltoken[STR_SIZE];

	if (!str)
		return NULL;

	if ((sp = parse_line(str, ltoken, sizeof(ltoken))) == NULL)
		return NULL;

	if ((rtoken = strtok(sp, "\t ,")) == NULL)
		return NULL;
	do {
		char *ip_str = NULL;
		unsigned long mask = 0;

		if (!strcmp(rtoken, "0.0.0.0") ||
				!strcmp(rtoken, "::") ||
				!strcmp(rtoken, "::0"))
		{
			continue;
		}
		if (parse_ip(rtoken, &ip_str, (unsigned int *)&mask))
			continue;
		if (ListFind(l, ip_str) != NULL)
			continue;
		if (mask != 0)
			l = ListAddIpElem(l, ip_str, &mask);
		else
			l = ListAddIpElem(l, ip_str, NULL);
		free(ip_str);
	} while ((rtoken = strtok(NULL, "\t ,")));

	return l;
}

int MemCmp(void *param1, void *param2, int len)
{
	if (param1 == NULL || param2 == NULL)
		return 1;
	return memcmp(param1, param2, len);
}

int WriteConfig(char *path, struct CList *cfile)
{
	struct CList *l = cfile;
	char buf[STR_SIZE];
	int fd;
	int len;
	int ret = 1;

	if (path == NULL)
	{
		fd = STDOUT_FILENO;
	}
	else
	{
		snprintf(buf, sizeof(buf), "%s.tmp", path);
		if ((fd = open(buf, O_CREAT|O_WRONLY|O_TRUNC, 0644)) < 0)
		{
			logger(-1, errno,
				"Unable to create the configuration file %s", path);
			return 1;
		}
	}
	while (l)
	{
		if (l->str)
		{
			len = strlen(l->str);
			ret = write(fd, l->str, len);
			if (ret < 0)
			{
				logger(-1, errno, "Unable to write %d"
					" bytes to %s",	len, buf);
				goto err;
			}
			if (!strrchr(l->str, '\n'))
				write(fd, "\n", 1);
		}
		l = l->next;
	}
	if (path != NULL)
	{
		fsync(fd);
		if (close(fd)) {
			logger(-1, errno, "Failed to close fd %s",
				buf);
			goto err;
		}

		if (rename(buf, path)) {
			logger(-1, errno, "Unable to rename %s -> %s",
				buf, path);
			goto err;
		}
	}
	ret = 0;
err:
	if (path != NULL) {
		if (ret)
			 unlink(buf);
		close(fd);
	}
	return ret;
}

int FillParam_tv(struct CList *conf, unsigned long *val, char *name, int mul)
{
	char tmp[STR_SIZE];
	struct CList *p, *pconf, *list = NULL;
	char *sp, *ep;

	if (val == NULL || name == NULL)
		return 0;
	sp = tmp;
	ep = tmp + sizeof(tmp);
	snprintf(sp, ep - sp, "%s=\"%llu:%llu\"\n", name,
		(unsigned long long) val[0] * mul,
		(unsigned long long) val[1] * mul);
	if ((pconf = ListFindVar(conf, name, '=')) == NULL) {
		pconf = ListAddElem(conf, NULL, NULL, NULL);
		pconf = ListLast(conf);
	}
	free(pconf->str);
	pconf->str = strdup(tmp);
	p = list = GetAliases(name);
	while (p) {
		if ((pconf = ListFindVar(conf, p->str, '=')) != NULL) {
			free(pconf->str);
			pconf->str = NULL;
		}
		p = p->next;
	}
	ListFree(list);
	return 0;
}

char *MergeTwoList(struct CList *config, struct CList *ladd,
			struct CList *ldel, char *name)
{
	struct CList *l, *oldparam, *plist;
	char *val = NULL;
	char *buf;
	char *sp, *ep;
	int r;
	int ip_count = 0;
	int len = 1;

	if (ladd == NULL  && ldel == NULL)
		return NULL;
	/* Get ip addresses list from config file */
	if ((plist = ListFindVar(config, name, '=')))
		val = plist->str;
	oldparam = ParseVariable(val);
	/* Calculate buf len */
	for (l = oldparam; l != NULL; l = l->next)
		if (l->str != NULL)
			len += strlen(l->str) + 1;
	for (l = ladd; l != NULL; l = l->next)
		if (l->str != NULL)
			len += strlen(l->str) + 1;
	if ((buf = (char*) xmalloc(len)) ==  NULL)
		return NULL;
	*buf = 0;
	sp = buf;
	ep = buf + len;
	l = oldparam;
	while (l)
	{
		/* Remove from config ips being  deleted  */
		if (strcmp(l->str, "0.0.0.0") &&
			!ListFind(ldel, l->str))
		{
			r = snprintf(sp, ep - sp, "%s ", l->str);
			if (r < 0 || sp + r >= ep)
			{
				break;
			}
			sp += r;
			ip_count++;
		}
		l = l->next;
	}
	l = ladd;
	while (l)
	{
		/* Add to config ips being successfully added to Container */
		if (!ListFind(oldparam, l->str) || ListFind(ldel, l->str))
		{
			r = snprintf(sp, ep - sp, "%s ", l->str);
			if (r < 0 || sp + r >= ep)
			{
				break;
			}
			sp += r;
			ip_count++;
		}
		l = l->next;
	}
	if (buf != sp)
		buf[sp - buf - 1] = 0;
	ListFree(oldparam);
	return buf;
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

char *MergeTwoIpList(struct CList *config, struct CList *ladd,
			struct CList *ldel, char *name)
{
	struct CList *l, *oldparam, *plist;
	char *val = NULL;
	char *buf;
	char *sp, *ep;
	int r;
	int len = 1;
	char ip_str[128];

	if (ladd == NULL  && ldel == NULL)
		return NULL;
	/* Get ip addresses list from config file */
	if ((plist = ListFindVar(config, name, '=')))
		val = plist->str;
	oldparam = ParseIpVariable(val);
	/* Calculate buf len */
	for (l = oldparam; l != NULL; l = l->next)
		if (l->str != NULL)
			len += strlen(l->str) + 16 + 1;
	for (l = ladd; l != NULL; l = l->next)
		if (l->str != NULL)
			len += strlen(l->str) + 16 + 1;
	if ((buf = (char*) xmalloc(len)) ==  NULL)
		return NULL;
	*buf = 0;
	sp = buf;
	ep = buf + len;
	for (l = oldparam; l != NULL; l = l->next)
	{
		/* Remove from config ips being deleted  */
		if (strcmp(l->str, "0.0.0.0") &&
			!ListFind(ldel, l->str))
		{
			struct CList *ip = ListFind(ladd, l->str);

			if (ip == NULL)
				ip = l;
			if (get_ip_str(ip, ip_str, sizeof(ip_str)))
				continue;
			r = snprintf(sp, ep - sp, "%s ", ip_str);
			if (r < 0 || sp + r >= ep)
			{
				break;
			}
			sp += r;
		}
	}
	for (l = ladd; l != NULL; l = l->next)
	{
		/* Add to config ips being successfully added to Container */
		if (!ListFind(oldparam, l->str) || ListFind(ldel, l->str))
		{
			if (get_ip_str(l, ip_str, sizeof(ip_str)))
				continue;
			r = snprintf(sp, ep - sp, "%s ", ip_str);
			if (r < 0 || sp + r >= ep)
			{
				break;
			}
			sp += r;
		}
	}
	if (buf != sp)
		buf[sp - buf - 1] = 0;
	ListFree(oldparam);
	return buf;
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
	case NETIF_ADD_T:
	{
		veth_param **p = (veth_param **) Config[i].pvar;

		if (*p == NULL) {
			*p = xcalloc(1, sizeof(veth_param));
			list_head_init(&(*p)->dev);
		}
		if (unset)
			break;
		ret = parse_netif_cmd(*p, sp);
		if (ret)
			return ret;
		break;
	}
	case NETIF_DEL_T:
	{
		veth_param **p = (veth_param **) Config[i].pvar;
		veth_dev dev;

		if (*p == NULL) {
			*p = xcalloc(1, sizeof(veth_param));
			list_head_init(&(*p)->dev);
		}
		if (unset)
			break;
		memset(&dev, 0, sizeof(dev));
		list_head_init(&dev.ip);
		if (strlen(sp) > IFNAMSIZE)
			return ERR_INVAL;
		if (!strcmp(sp, "all")) {
			(*p)->delall = YES;
			return 0;
		}
		if (find_veth_by_ifname_ve(&(*p)->dev, sp) != NULL)
			return 0;
		strcpy(dev.dev_name_ve, sp);
		ret = add_veth_param(*p, &dev);
		if (ret)
			return ret;
		break;
	}
	case NETIF_NAME:
	case NETIF_MAC:
	case NETIF_HOST_IFNAME:
	case NETIF_HOST_MAC:
	case NETIF_GW:
	case NETIF_GW6:
	case NETIF_DHCP:
	case NETIF_DHCP6:
	case NETIF_MAC_FILTER:
	case NETIF_CONFIGURE_MODE:
	case NETIF_NETWORK:
	{
		veth_param **p = (veth_param **) Config[i].pvar;

		if (*p == NULL) {
			*p = xcalloc(1, sizeof(veth_param));
			list_head_init(&(*p)->dev);
		}
		if (unset)
			break;
		ret = add_netif_param(*p, Config[i].type, sp);
		if (ret)
			return ret;
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

int get_run_ve(int **buf)
{
	int ret;
	struct vzlist_veidctl veid;
	int fd;

	fd = open(VZCTLDEV, O_RDWR);
	if (fd < 0)
		return -1;;
	veid.num = 256;
	(*buf) = xmalloc(veid.num * sizeof(envid_t));
	while (1) {
		veid.id = (unsigned *)*buf;
		ret = ioctl(fd, VZCTL_GET_VEIDS, &veid);
		if (ret < 0)
			goto err;
		if (ret <= veid.num)
			break;
		veid.num = ret + 20;
		(*buf) = realloc(*buf, veid.num * sizeof(int));
	}
	qsort(*buf, ret, sizeof(int), s32_comp);
err:
	close(fd);
	return ret;
}

int *find_veid(int veid, int *base, int nmemb)
{
	if (base == NULL || nmemb <= 0)
		return NULL;
	return bsearch(&veid, base, nmemb, sizeof(int), s32_comp);
}
