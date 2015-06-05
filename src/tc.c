#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vzctl/libvzctl.h>
#include <linux/vzctl_venet.h>
#include <linux/vzctl_netstat.h>

#include "tc.h"

extern int __vzctlfd;

int get_ipaddr(char *name, unsigned int *ip)
{
	struct in_addr addr;

	if (name == NULL)
		return ERR_BAD_IP;
	if (!inet_aton(name, &addr))
		return ERR_BAD_IP;
	*ip = addr.s_addr;
	return 0;
}

int tc_set_classes(struct vz_tc_class_info *info, int length)
{
	struct vzctl_tc_classes classes;

	classes.info = info;
	classes.length = length;
	if (ioctl(__vzctlfd, VZCTL_TC_SET_CLASS_TABLE, &classes))
	{
		fprintf(stderr, "Unable to set classes: %s\n", strerror(errno));
		return ERR_IOCTL;
	}
	return 0;
}

int tc_set_v6_classes(struct vz_tc_class_info_v6 *info, int length)
{
	struct vzctl_tc_classes_v6 classes;

	classes.info = info;
	classes.length = length;
	if (ioctl(__vzctlfd, VZCTL_TC_SET_CLASS_TABLE_V6, &classes))
	{
		if (errno == ENOTTY && length == 0)
			return 0;
		fprintf(stderr, "Unable to set ipv6 classes: %s\n", strerror(errno));
		return ERR_IOCTL;
	}
	return 0;
}

int tc_get_classes(struct vz_tc_class_info *info, int length)
{
	struct vzctl_tc_classes classes;
	int ret;

	classes.info = info;
	classes.length = length;
	if ((ret = ioctl(__vzctlfd, VZCTL_TC_GET_CLASS_TABLE, &classes)) < 0)
	{
		fprintf(stderr, "Unable to get classes: %s\n", strerror(errno));
		return ERR_IOCTL;
	}
	return ret;
}

int tc_get_v6_classes(struct vz_tc_class_info_v6 *info, int length)
{
	struct vzctl_tc_classes_v6 classes;
	int ret;

	classes.info = info;
	classes.length = length;
	if ((ret = ioctl(__vzctlfd, VZCTL_TC_GET_CLASS_TABLE_V6, &classes)) < 0)
	{
		if (errno == ENOTTY)
			return 0;
		fprintf(stderr, "Unable to get ipv6 classes: %s\n", strerror(errno));
		return ERR_IOCTL;
	}
	return ret;
}

int tc_get_stat(struct vzctl_tc_get_stat *stat)
{
	int ret;

	if ((ret = ioctl(__vzctlfd, VZCTL_TC_GET_STAT, stat)) < 0)
	{
		if (errno != ESRCH)
		{
			fprintf(stderr, "Unable to get Container %d statistics: %s\n",
				stat->veid, strerror(errno));
			return ERR_IOCTL;
		}
		ret = 0;
	}
	return ret;
}

int tc_get_v6_stat(struct vzctl_tc_get_stat *stat)
{
	int ret;

	if ((ret = ioctl(__vzctlfd, VZCTL_TC_GET_STAT_V6, stat)) < 0)
	{
		if (errno != ESRCH)
		{
			fprintf(stderr, "Unable to get Container %d statistics: %s\n",
				stat->veid, strerror(errno));
			return ERR_IOCTL;
		}
		ret = 0;
	}
	return ret;
}
int tc_destroy_ve_stats(ctid_t ctid)
{
	int ret;
	struct vzctl_env_handle *h;

	if (!(h = vzctl2_env_open(ctid, 0, &ret)))
		return ret;

	ret = vzctl2_clear_ve_netstat(h);
	if (ret) {
		fprintf(stderr, "Reset failed: %s\n", strerror(errno));
		return ERR_IOCTL;
	}

	printf("Counters for Container %s reseted.\n", ctid);
	vzctl2_env_close(h);

	return 0;
}

int tc_destroy_all_stats(void)
{
	if (vzctl2_clear_all_ve_netstat())
	{
		fprintf(stderr, "Reset failed: %s\n", strerror(errno));
		return ERR_IOCTL;
	}

	printf("Counters reseted.\n");
	return 0;
}

int tc_get_class_num(void)
{
	int len;

	if ((len = ioctl(__vzctlfd, VZCTL_TC_CLASS_NUM, NULL)) < 0)
	{
		if (errno == ENOTTY)
			return 0;
		fprintf(stderr, "Unable get classes count: %s\n",
				strerror(errno));
		return ERR_IOCTL;
	}
	return len;
}

int tc_get_v6_class_num(void)
{
	int len;

	if ((len = ioctl(__vzctlfd, VZCTL_TC_CLASS_NUM_V6, NULL)) < 0)
	{
		if (errno == ENOTTY)
			return 0;
		fprintf(stderr, "Unable to get v6 classes count: %s\n",
				strerror(errno));
		return ERR_IOCTL;
	}
	return len;
}

int tc_get_base(int veid)
{
	return ioctl(__vzctlfd, VZCTL_TC_GET_BASE, veid);
}

int tc_max_class(void)
{
	int len;

	len = ioctl(__vzctlfd, VZCTL_TC_MAX_CLASS, NULL);
	if (len < 0)
	{
		fprintf(stderr, "Unable get max classes: %s\n", strerror(errno));
		return ERR_IOCTL;
	}
	return len;
}

int tc_get_stat_num(void)
{
	int len;

	if ((len = ioctl(__vzctlfd, VZCTL_TC_STAT_NUM, NULL)) < 0)
	{
		fprintf(stderr, "Unable get Container count in accounting table: %s\n",
			strerror(errno));
		return ERR_IOCTL;
	}
	return len;
}

int tc_get_ve_list(struct vzctl_tc_get_stat_list *velist)
{
	int ret;

	if ((ret = ioctl(__vzctlfd, VZCTL_TC_GET_STAT_LIST, velist)) < 0)
	{
		fprintf(stderr, "Unable to get Container list: %s\n",
				strerror(errno));
		return ERR_IOCTL;
	}
	velist->length = ret;
	return ret;
}

int get_ve_list(struct vzctl_tc_get_stat_list *velist)
{
	int len;

	if ((len = tc_get_stat_num()) < 0)
		return len;
	if (!len )
		return 0;
	velist->list = calloc(len, sizeof(int));
	velist->length = len;
	if ((len = tc_get_ve_list(velist)) < 0)
		return len;
	return len;
}
