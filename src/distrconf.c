#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "distrconf.h"
#include "vzerror.h"
#include "vzctl.h"
#include "tmplmn.h"
#include "config.h"
#include "util.h"

dist_actions *distActions = NULL;

struct distr_conf {
	char *name;
	int id;
} action2id[] = {
	{"ADD_IP", ADD_IP},
	{"DEL_IP", DEL_IP},
	{"SET_HOSTNAME", SET_HOSTNAME},
	{"SET_DNS", SET_DNS},
	{"SET_USERPASS", SET_USERPASS},
	{"SET_UGID_QUOTA", SET_UGID_QUOTA},
	{"SET_CONSOLE", SET_CONSOLE},
	{"POST_CREATE", POST_CREATE},
	{"NETIF_ADD", NETIF_ADD},
	{"NETIF_DEL", NETIF_DEL},
};

static int get_action_id(char *name)
{
	int i;

	for (i = 0; i < sizeof(action2id) / sizeof(*action2id); i++)
		if (!strcmp(name, action2id[i].name))
			return action2id[i].id;
	return -1;
}

static int add_dist_action(dist_actions *d_actions, char *name,
	char *action, char *dir)
{
	char file[256];
	int id;

	if (!action[0])
		return 0;
	if ((id = get_action_id(name)) < 0)
		return 0;
	snprintf(file, sizeof(file), "%s/%s", dir, action);
	if (!stat_file(file)) {
		logger(-1, 0, "Warning: %s action script (%s) is not found",
			name, file);
		return VZ_NO_DISTR_ACTION_SCRIPT;
	}
#define ADD_DIST_SCRIPT(name, file)				\
	if (d_actions->name == NULL)			\
		d_actions->name = strdup(file);

	switch (id) {
		case ADD_IP:
			ADD_DIST_SCRIPT(add_ip, file)
			break;
		case DEL_IP:
			ADD_DIST_SCRIPT(del_ip, file)
			break;
		case NETIF_ADD:
			ADD_DIST_SCRIPT(netif_add, file)
			break;
		case NETIF_DEL:
			ADD_DIST_SCRIPT(netif_del, file)
			break;
		case SET_HOSTNAME:
			ADD_DIST_SCRIPT(set_hostname, file)
			break;
		case SET_DNS:
			ADD_DIST_SCRIPT(set_dns, file)
			break;
		case SET_USERPASS:
			ADD_DIST_SCRIPT(set_userpass, file)
			break;
		case SET_UGID_QUOTA:
			ADD_DIST_SCRIPT(set_ugid_quota, file)
			break;
		case SET_CONSOLE:
			ADD_DIST_SCRIPT(set_console, file)
			break;
		case POST_CREATE:
			ADD_DIST_SCRIPT(post_create, file)
			break;
	}
#undef ADD_DIST_SCRIPT
	return 0;
}

void free_dist_actions(void)
{
	if (distActions == NULL)
		return;
	free(distActions->add_ip);
	free(distActions->del_ip);
	free(distActions->set_hostname);
	free(distActions->set_dns);
	free(distActions->set_userpass);
	free(distActions->set_ugid_quota);
	free(distActions->set_console);
	free(distActions->post_create);
}

static int get_dist_conf_name(char *dist_name, char *dir, char *file,
	int len)
{
	char buf[512];
	char *ep;

	if (dist_name != NULL) {
		snprintf(buf, sizeof(buf), "%s", dist_name);
		ep = buf + strlen(buf);
		do {
			snprintf(file, len, "%s/%s.conf", dir, buf);
			if (stat_file(file))
				return 0;
			while (ep > buf && *ep !=  '-') --ep;
			*ep = 0;
		} while (ep > buf);
		snprintf(file, len, "%s/%s", dir, DIST_CONF_DEF);
		logger(-1, 0, "Warning: configuration file"
			" for the distribution %s is not found; the default distribution is used",
			dist_name);
	} else {
		snprintf(file, len, "%s/%s", dir, DIST_CONF_DEF);
		logger(-1, 0, "Warning: Distribution is not specified"
			"; the default distribution (%s) is used", file);
	}
	if (!stat_file(file)) {
		logger(-1, 0, "Distribution's configuration could not be found %s",
			file);
		return VZ_NO_DISTR_CONF;
	}
	return 0;
}

int read_dist_actions(void)
{
	char buf[256];
	char ltoken[256];
	char file[256];
	char *rtoken;
	FILE *fp;
	int ret = 0;
	char *dist_name;

	if (distActions != NULL)
		return 0;
	distActions = xmalloc(sizeof(*distActions));
	memset(distActions, 0, sizeof(*distActions));
	dist_name = get_dist_type(gparam);
	if ((ret = get_dist_conf_name(dist_name, DIST_CONF_DIR, file,
		sizeof(file))))
	{
		return ret;
	}
	if ((fp = fopen(file, "r")) == NULL) {
		logger(-1, errno, "Unable to open %s", file);
		return VZ_NO_DISTR_CONF;
	}
	while (!feof(fp)) {
		buf[0] = 0;
		if (fgets(buf, sizeof(buf), fp) == NULL)
			break;
		if ((rtoken = parse_line(buf, ltoken, sizeof(ltoken))) == NULL)
			continue;
		add_dist_action(distActions, ltoken, rtoken,
			DIST_SCRIPTS);
	}
	fclose(fp);
	free(dist_name);
	return ret;
}

int get_dist_action(char *name, char **script)
{
	char buf[256];
	char ltoken[256];
	char file[256];
	char *rtoken;
	FILE *fp;
	int ret = 0;
	char *dist_name;

	dist_name = get_dist_type(gparam);
	if ((ret = get_dist_conf_name(dist_name, DIST_CONF_DIR, file,
		sizeof(file))))
	{
		return ret;
	}
	if ((fp = fopen(file, "r")) == NULL) {
		logger(-1, errno, "Unable to open %s", file);
		return VZ_NO_DISTR_CONF;
	}
	while (!feof(fp)) {
		buf[0] = 0;
		if (fgets(buf, sizeof(buf), fp) == NULL)
			break;
		if ((rtoken = parse_line(buf, ltoken, sizeof(ltoken))) == NULL)
			continue;
		if (!strcmp(ltoken, name)) {
			snprintf(file, sizeof(file), DIST_SCRIPTS "%s", rtoken);
			*script = strdup(file);
			break;
		}
	}
	fclose(fp);
	if (*script == NULL) {
		logger(-1, 0, "Action %s is not found for %s",
			name, dist_name);
		return VZ_NO_DISTR_CONF;
	}
	free(dist_name);
	return 0;
}
