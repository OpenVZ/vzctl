#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

#define VZ_2_0_2_CONFIG
#include "vzctl.h"
#include "config.h"
#include "util.h"

struct CParam *gparam, *lparam;

void usage()
{
	fprintf(stdout, "Usage: vzcfgconvert <veid>\n");
}

struct CParam *convert1to2(struct CParam *param)
{
	struct CParam *new = ConvertUBC(param);

	RemoveOldParam(param);
#define CHANGE_PARAM(name) \
	if (new->name != NULL) { \
		if (param->name == NULL) \
			param->name = malloc(sizeof(unsigned long) * 2); \
		param->name[0] = new->name[0]; \
		param->name[1] = new->name[1]; \
	}
	CHANGE_PARAM(kmemsize)
	CHANGE_PARAM(tcpsendbuf)
	CHANGE_PARAM(tcprcvbuf)

	FillParam(0, new, param);

	return new;
}

void convert2to3(struct CParam *param)
{
#define INC_PARAM(name, mul)		\
	if (param->name != NULL) {	\
		param->name[0] *= mul;	\
		param->name[1] *= mul;	\
	}				\

	INC_PARAM(kmemsize, 1.2)
	INC_PARAM(numfile, 1.6)
	INC_PARAM(dcachesize, 2)
}

int main(int argc, char**argv)
{
	char buf[STR_SIZE];
	char tmp[STR_SIZE];
	struct CParam param, *newparam = NULL;
	int veid;
	int ver = 0;
	struct stat st;

	if (argc != 2) {
		usage();
		exit(1);
	}
	if (parse_int(argv[1], &veid))
	{
		fprintf(stderr, "Error: Invalid veid: %s\n", argv[1]);
		return 1;
	}
	snprintf(buf, sizeof(buf), "%s/%d.conf", CFG_DIR, veid);
	if (stat(buf, &st))
	{
		fprintf(stderr, "Error: Container config file %s not found\n", buf);
		return 1;
	}
	gparam = &param;
	memset(&param, 0, sizeof(struct CParam));
	gparam->log_enable = YES;
	gparam->quiet = 1;
	if (ParseConfig(veid, buf, &param, 1))
		return 1;
	if (param.version == 3)
	{
		fprintf(stderr, "Nothing to convert Container %d config file is new\n"
				, veid);
		return 0;
	}
	else if (param.version != 2)
	{
		ver = GetConfVer(veid, param.version, &param);
		if (ver == -1)
		{
			fprintf(stderr, "Error: Unable to determine Container config"
						" version. Invalid config\n");
			return 1;
		} else if (ver == 1) {
			newparam = convert1to2(&param);
		} else {
			newparam = &param;
		}
	} else {
		newparam = &param;
	}
	newparam->version = 2;
	convert2to3(newparam);
	sprintf(tmp, "%s.tmp", buf);
	if (ver != 1)
		copy_file(buf, tmp);
	if (SaveConfig(veid, tmp, newparam, NULL) < 0)
	{
		fprintf(stderr, "Error: Unable to save temporary Container config"
			" file %s\n", tmp);
		return 1;
	}
	rename(tmp, buf);
	fprintf(stderr, "Container %d config file converted: success\n", veid);

	return 0;
}
