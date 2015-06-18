#include <stdio.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <vzctl/libvzctl.h>
#include "vzctl.h"

#ifndef NETLINK_VZEVENT
#define NETLINK_VZEVENT		31
#endif

void msg_print(char *buf, ctid_t ctid)
{
	char *p;

	p = strchr(buf, '@');
	if (p == NULL || ++p == 0) {
		logger(0, 0, "Invalid message format: [%s]", buf);
		return;
	}

	if (CMP_CTID(p, ctid) == 0)
		fprintf(stdout, "%s\n",  buf);
}

int monitoring(ctid_t ctid)
{
	int s, ret;
	struct sockaddr_nl nladdr;
	struct iovec iov;
	char buf[512];
	struct msghdr msg;

	ret = -1;
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void*)&nladdr;
	msg.msg_namelen = sizeof(nladdr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_VZEVENT);
	if (s < 0)
		goto out;

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_groups = 0x01; /* multicast */

	if (bind(s, (struct sockaddr *)&nladdr, sizeof(nladdr)) == -1) {
		logger(-1, errno, "Could not bind the socket");
		goto out;
	}

	nladdr.nl_pid = 0;
	nladdr.nl_groups = 0;

	while (1) {
		memset(buf, 0, sizeof(buf));
		iov.iov_base = buf;
		iov.iov_len = sizeof(buf);

		ret = recvmsg(s, &msg, 0);
		if (ret < 0) {
			logger(-1, errno, "Failed to receive the event message");
			goto out;
		} else if (!ret)
			break;

		msg_print(buf, ctid);
	}
	ret = 0;
out:
	return ret;
}
