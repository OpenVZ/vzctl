/*
 * Copyright (c) 1999-2017, Parallels International GmbH
 *
 * This file is part of OpenVZ. OpenVZ is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "vzcfgscale.h"
#include "vzctl.h"
#include "config.h"
#include "validate.h"

struct CParam *gparam, *lparam;

static struct option options[] =
{
	{"output-file",	required_argument, NULL, PARAM_OUTPUT_FILE},
	{"all",		required_argument, NULL, PARAM_ALL},
	{"cpu-only",	required_argument, NULL, PARAM_CPU},
	{"disk-only",	required_argument, NULL, PARAM_DISK},
	{"ubc-only",	required_argument, NULL, PARAM_UBC},
	{"network-only",required_argument, NULL, PARAM_NET},
	{"remove",	no_argument, NULL, PARAM_REMOVE},
	{"help",	no_argument, NULL, PARAM_HELP},
	{ NULL, 0, NULL, 0 }
};

void usage(void)
{
	fprintf(stderr, "Usage: vzcfgscale [options] <configfile>\n"
"Options are:\n"
"\t-o, --output-file <file>   Write output to file. Default is stdout\n"
"\t-a, --all <coeff>          Scale all parameters to the value <coeff>\n"
"\t-c, --cpu-only <coeff>     Scale only CPU parameters\n"
"\t-d, --disk-only <coeff>    Scale only disk quota parameters\n"
"\t-u, --ubc-only <coeff>     Scale only UBC parameters\n"
"\t-n, --network-only <coeff> Scale only network bandwidth parameters\n"
"\t-r, --remove               Removes Container-specific parameters\n");
}

void remove_ve_spec(struct CParam *param)
{
#define FREE_PARAM(name) free(param->name); param->name = NULL;

	FREE_PARAM(hostname)
	FREE_PARAM(nameserver)
	FREE_PARAM(searchdomain)
	FREE_PARAM(tmpl_set)
	FREE_PARAM(ve_root_orig)
	FREE_PARAM(ve_private_orig)
	FREE_PARAM(templates)
	ListFree(param->ipadd);
	param->ipadd =  NULL;
	param->vetype = 0;
}

void scale(float ubc_k, float cpu_k, float disk_k, float net_k,
		struct CParam *param)
{
	unsigned long val0, val1;

#define SCALE_UBC(name, k) \
do { \
	if (param->name == NULL) \
		break; \
	val0 = param->name[0]; \
	val1 = param->name[1]; \
	if (val0 != LONG_MAX) \
		val0 *= k; \
	if (val0 > LONG_MAX) \
		val0 = LONG_MAX; \
	if (val1 != LONG_MAX) \
		val1 *= k; \
	if (val1 > LONG_MAX) \
		val1 = LONG_MAX; \
	param->name[0] = val0; \
	param->name[1] = val1; \
} while(0);

#define SCALE_PARAM(name, k) \
do { \
	if (param->name == NULL) \
		break; \
	param->name[0] *= k; \
	param->name[1] *= k; \
	if (param->name[0] > LONG_MAX) \
		param->name[0] = LONG_MAX; \
	if (param->name[1] > LONG_MAX) \
		param->name[1] = LONG_MAX; \
} while(0);

	if (ubc_k)
	{
		SCALE_UBC(kmemsize, ubc_k)
		SCALE_UBC(avnumproc, ubc_k)
		SCALE_UBC(numproc, ubc_k)
		SCALE_UBC(vmguarpages, ubc_k)
		SCALE_UBC(oomguarpages, ubc_k)
		SCALE_UBC(lockedpages, ubc_k)
		SCALE_UBC(shmpages, ubc_k)
		SCALE_UBC(privvmpages, ubc_k)
		SCALE_UBC(numfile, ubc_k)
		SCALE_UBC(numflock, ubc_k)
		SCALE_UBC(numpty, ubc_k)
		SCALE_UBC(numsiginfo, ubc_k)
		SCALE_UBC(dcachesize, ubc_k)
		SCALE_UBC(physpages, ubc_k)
		SCALE_UBC(numtcpsock, ubc_k)
		SCALE_UBC(numothersock, ubc_k)
		SCALE_UBC(tcpsndbuf, ubc_k)
		SCALE_UBC(tcprcvbuf, ubc_k)
		SCALE_UBC(othersockbuf, ubc_k)
		SCALE_UBC(dgramrcvbuf, ubc_k)
		SCALE_UBC(numiptent, ubc_k)
		SCALE_UBC(slmmemorylimit, ubc_k)
	}
	if (cpu_k && param->cpu_units != NULL)
	{
		*param->cpu_units *= cpu_k;
		if (*param->cpu_units > MAXCPUUNITS)
			 *param->cpu_units = MAXCPUUNITS;
	}
	if (disk_k)
	{
		SCALE_PARAM(quota_block, disk_k)
		SCALE_PARAM(quota_inode, disk_k)
	}
	if (net_k && param->rate != NULL)
	{
		struct CList *p;

		for (p = param->rate; p != NULL; p = p->next)
		{
			if (p->val2 == NULL)
				continue;
			*p->val2 *= net_k;
			if (*p->val2 > LONG_MAX)
				*p->val2 = LONG_MAX;
		}
	}
}

int check_file(char *file1, char *file2)
{
	struct stat st1, st2;

	if (file1 == NULL || file2 == NULL)
		return 0;
	if (stat(file1, &st1))
		return 0;
	if (stat(file2, &st2))
		return 0;
	if (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino)
		return 1;
	return 0;
}

int get_id(char *in_file)
{
	char buf[STR_SIZE];
	char *p;
	int id;

	if (in_file == NULL)
		return 0;
	if ((p = strrchr(in_file, '/')) != NULL)
		p++;
	else
		p = in_file;
	if (sscanf(p, "%d.conf", &id) != 1)
		return 0;
	snprintf(buf, sizeof(buf), SCRIPT_DIR "%d.conf", id);
	if (check_file(in_file, buf))
		return id;
	return 0;
}

int cpfile(char *in, char *out)
{
	char buf[STR_SIZE];

	if (in == NULL || out == NULL)
		return 0;
	if (check_file(in, out))
		return 0;
	snprintf(buf, sizeof(buf), "/bin/cp -f %s %s", in, out);
	if (system(buf))
		return 1;
	return 0;
}

int main(int argc, char **argv)
{
	struct CParam param;
	struct stat st;
	char buf[STR_SIZE];
	char *in_file = NULL;
	char *out_file = NULL;
	float ubc_k, cpu_k, disk_k, net_k, all_k;
	int remove;
	int veid, c;

	gparam = &param;
	memset(&param, 0, sizeof(param));
	ubc_k = cpu_k = disk_k = net_k = all_k = 0;
	remove = 0;
	if (argc < 2)
	{
		usage();
		exit(1);
	}
	while(1)
	{
		int option_index = -1;
		c = getopt_long(argc, argv, "o:a:c:d:u:n:hrv", options,
				&option_index);
		if (c == -1)
			break;
		switch(c)
		{
			case PARAM_OUTPUT_FILE	:
				out_file = strdup(optarg);
				break;
			case PARAM_ALL		:
				if (sscanf(optarg, "%f", &all_k) != 1)
				{
					fprintf(stderr, "Bad parameter %s\n",
							optarg);
					return 1;
				}
				break;
			case PARAM_CPU		:
				if (sscanf(optarg, "%f", &cpu_k) != 1)
				{
					fprintf(stderr, "Bad parameter %s\n",
							optarg);
					return 1;
				}
				break;
			case PARAM_DISK		:
				if (sscanf(optarg, "%f", &disk_k) != 1)
				{
					fprintf(stderr, "Bad parameter %s\n",
							optarg);
					return 1;
				}
				break;
			case PARAM_UBC		:
				if (sscanf(optarg, "%f", &ubc_k) != 1)
				{
					fprintf(stderr, "Bad parameter %s\n",
							optarg);
					return 1;
				}
				break;
			case PARAM_NET		:
				if (sscanf(optarg, "%f", &net_k) != 1)
				{
					fprintf(stderr, "Bad parameter %s\n",
							optarg);
					return 1;
				}
				break;
			case PARAM_REMOVE	:
				remove = 1;
				break;
			case PARAM_HELP		:
				usage();
				exit(0);
			case '?'		:
				exit(1);
			default			:
				fprintf(stderr, "Unknown option %s\n", optarg);
				exit(1);
		}

	}
	if (optind == argc - 1)
	{
			in_file = strdup(argv[optind]);
	}
	else if (optind < argc)
	{
		fprintf(stderr, "non-option ARGV-elements: ");
		while (optind < argc - 1)
			fprintf(stderr, "%s ", argv[optind++]);
		fprintf(stderr, "\n");
		exit(1);
	}
	if (in_file == NULL)
	{
		usage();
		exit(1);
	}
	if (!stat(out_file, &st) && S_ISDIR(st.st_mode))
	{
		printf("Error: output file %s is directory\n", out_file);
		return 1;
	}
	if (all_k)
		ubc_k = cpu_k = disk_k = net_k =  all_k;
	if (stat(in_file, &st))
	{
		fprintf(stderr, "No such file %s\n", in_file);
		exit(1);
	}
	vzctl_init_log(0, 0, NULL);
	if (ParseConfig(0, in_file, &param, 0))
	{
		fprintf(stderr, "Unable open %s\n", in_file);
		return 1;
	}
	scale(ubc_k, cpu_k, disk_k, net_k, &param);
	if (remove)
		remove_ve_spec(&param);

	if ((veid = get_id(in_file)))
	{
		if (!vzctl_lib_init() && env_is_running(veid))
		{
			fprintf(stderr, "Container %d is running, apply parameters\n",
				veid);
			if (ubc_k)
				mm_configure(veid, &param);
			if (cpu_k)
				vzctl_set_cpu_param(veid, param.vcpu,
								param.cpu_weight,
								param.cpu_units,
								param.cpu_limit,
								param.cpumask,
								param.nodemask);
		}
	}
	/* if out_file already exist make backup */
	if (out_file != NULL && !stat(out_file, &st))
	{
		snprintf(buf, sizeof(buf), "%s.bak", out_file);
		if (remove)
			rename(out_file, buf);
		else
			cpfile(out_file, buf);
	}
	if (out_file != NULL && !remove)
		cpfile(in_file, out_file);
	if (SaveConfig(0, out_file, &param, NULL) == -1)
		return 1;
	fprintf(stderr, "Scale completed: success\n");
	return 0;
}
