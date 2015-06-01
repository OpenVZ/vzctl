#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/utsname.h>

#include "vzctl.h"

#define VE_FEATURE_SYSFS	(1ULL << 0)
#define VE_FEATURE_NFS		(1ULL << 1)
#define VE_FEATURE_DEF_PERMS	(1ULL << 2)
#define VE_FEATURE_SIT		(1ULL << 3)
#define VE_FEATURE_IPIP		(1ULL << 4)
#define VE_FEATURE_PPP		(1ULL << 5)
#define VE_FEATURE_IPGRE	(1ULL << 6)
#define VE_FEATURE_BRIDGE	(1ULL << 7)
#define VE_FEATURE_NFSD		(1ULL << 8)

struct env_feature {
	char *name;
	unsigned long long mask;
};
static struct env_feature env_features[] = {
	{ "nfs",	VE_FEATURE_NFS},
	{ "sit",	VE_FEATURE_SIT},
	{ "ipip",	VE_FEATURE_IPIP},
	{ "ppp",	VE_FEATURE_PPP},
	{ "ipgre",	VE_FEATURE_IPGRE},
	{ "bridge",	VE_FEATURE_BRIDGE},
	{ "nfsd",	VE_FEATURE_NFSD},

	{ NULL}
};

void features_mask2str(unsigned long long mask, unsigned long long known, char *delim,
		char *buf, int len)
{
	struct env_feature *feat;
	int ret, i, j = 0;

	for (i = 0, feat = env_features; feat->name != NULL; feat++) {
		if (!(known & feat->mask))
			continue;
		ret = snprintf(buf, len, "%s%s:%s",
				j++ == 0 ? "" : delim,
				feat->name,
				mask & feat->mask ? "on" : "off");
		buf += ret;
		len -= ret;
		if (len <= 0)
			break;
		i++;
	}
}
