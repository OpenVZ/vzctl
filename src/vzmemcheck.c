#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

#include "vzctl.h"
#include "config.h"
#include "validate.h"

struct CParam *gparam, *lparam;

void usage(void)
{
	printf("Usage: vzmemcheck [-v] [-A]\n");
}

void header(int verbose, int param)
{
	if (param)
		printf("Output values in Mbytes\n");
	else
		printf("Output values in %%\n");
	if (verbose)
		printf("veid       ");
	printf(" LowMem  LowMem     RAM MemSwap MemSwap   Alloc   Alloc   Alloc\n");
	if (verbose)
		printf("           ");
	printf("   util  commit    util    util  commit    util  commit   limit\n");
	return;
}

struct CList *get_ve_ids(void)
{
	FILE *fd;
	char str[STR_SIZE * 20];
	struct CList *list = NULL;
	int res, id;

	fd = fopen(VEINFOPATH, "r");
	if (fd == NULL) {
		fprintf(stderr, "Can't open file %s\n", VEINFOPATH);
		return NULL;
	}
	while (fgets(str, sizeof(str), fd)) {
		if ((res = sscanf(str, "%d", &id)) == 1) {
			if (id) {
				sprintf(str, "%d", id);
				list = ListAddElem(list, str, NULL, NULL);
			}
		}
	}
	fclose(fd);
	return list;
}

void shift_param(struct CParam *param)
{
#define SHIFTPARAM(name)						\
if (param->name != NULL) {						\
	memcpy(param->name, &param->name[1], sizeof(unsigned long) * 2);\
}
	SHIFTPARAM(numproc);
	SHIFTPARAM(numtcpsock);
	SHIFTPARAM(numothersock);
	SHIFTPARAM(oomguarpages)
	SHIFTPARAM(vmguarpages);
	SHIFTPARAM(kmemsize);
	SHIFTPARAM(tcpsndbuf);
	SHIFTPARAM(tcprcvbuf);
	SHIFTPARAM(othersockbuf);
	SHIFTPARAM(dgramrcvbuf);
	SHIFTPARAM(privvmpages);
	SHIFTPARAM(numfile);
	SHIFTPARAM(dcachesize);
	SHIFTPARAM(physpages)
	SHIFTPARAM(numpty)
}

int calculate(int numerator, int verbose, int xmlout)
{
	struct CParam param;
	struct CRusage rutotal_comm, rutotal_utl;
	struct CRusage *ru_comm = NULL, *ru_utl = NULL;
	char str[STR_SIZE];
	char name[STR_SIZE];
	const char *fmt = NULL;
	int veid = 0, res, id, i, ret = 0, found = 0, new_veid = 0;
	int exited = 0;
	double r, rs, lm, k;
	unsigned long long lowmem;
	unsigned long long ram, swap = 1;
	unsigned long held, maxheld, barrier, limit;
	FILE *fd;
	int *veids = NULL, numve;

	numve = get_run_ve(&veids);
	fd = fopen(PROCUBC, "r");
	if (fd == NULL) {
		fprintf(stderr, "Can't open file %s\n", PROCUBC);
		return 1;
	}
	if (get_mem(&ram))
		return 1;
	get_swap(&swap);
	if (get_lowmem(&lowmem))
		return 1;
	lowmem *= 0.4;
	memset(&rutotal_comm, 0, sizeof(struct CRusage));
	memset(&rutotal_utl, 0, sizeof(struct CRusage));
	if (xmlout)
	{
		/* leave ratio for XML output */
		k = 1.0;
		numerator = 0;
	} else if (numerator) {
		/* Convert to Mb */
		k = 1.0 / (1024 * 1024);
	} else {
		/* Convert to % */
		k = 100;
	}
	if (!xmlout)
		header(verbose, numerator);
	memset(&tmpparam, 0, sizeof(struct CParam));
	while (!exited)
	{
		if (fgets(str, sizeof(str), fd) == NULL)
		{
			veid = new_veid;
			found = 1;
			str[0] = 0;
			exited = 1;
		}
		else
		{
			if ((res = sscanf(str, "%d:", &id)) == 1) {
				fmt =  "%*lu:%s%lu%lu%lu%lu";
				veid = new_veid;
				new_veid = id;
				found = 1;
			} else {
				fmt = "%s%lu%lu%lu%lu";
				found = 0;
			}
		}
		if (found && veid && !check_param(&tmpparam, 0) &&
		    find_veid(veid, veids, numve) != NULL)
		{
			memcpy(&param, &tmpparam, sizeof(struct CParam));
			if ((ru_utl = calc_ve_utilization(&param, numerator)))
			{
				ru_utl->low_mem *= k;
				ru_utl->total_ram *= k;
				ru_utl->mem_swap *= k;
				ru_utl->alloc_mem *= k;
			} else {
				ru_utl = malloc(sizeof(struct CRusage));
				memset(ru_utl, 0, sizeof(struct CRusage));
			}
			shift_param(&param);
			if ((ru_comm = calc_ve_commitment(veid, &param, numerator)))
			{
				ru_comm->low_mem *= k;
				ru_comm->total_ram *= k;
				ru_comm->mem_swap *= k;
				ru_comm->alloc_mem *= k;
				ru_comm->alloc_mem_lim *= k;
			} else {
				ru_comm = malloc(sizeof(struct CRusage));
				memset(ru_comm, 0, sizeof(struct CRusage));
			}
			if (verbose) {
				printf("%-10d %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f"
					" %7.2f\n", veid,
					ru_utl->low_mem,
					ru_comm->low_mem,
					ru_utl->total_ram,
					ru_utl->mem_swap,
					ru_comm->mem_swap,
					ru_utl->alloc_mem,
					ru_comm->alloc_mem,
					ru_comm->alloc_mem_lim);
			}
			inc_rusage(&rutotal_comm, ru_comm);
			inc_rusage(&rutotal_utl, ru_utl);
			FreeParam(&param);
			free(ru_utl);
			free(ru_comm);
			memset(&tmpparam, 0, sizeof(struct CParam));
		}
		if (!exited && (res = sscanf(str, fmt,
			name, &held, &maxheld, &barrier, &limit)) == 5)
		{
			for (i = 0; name[i] != 0; i++)
				name[i] = toupper(name[i]);
			if ((i = FindName(name)) >= 0) {
				unsigned long **par = Config[i].pvar;
				*par = (unsigned long *)malloc
					(sizeof(unsigned long) * 3);
				(*par)[0] = held;
				(*par)[1] = barrier;
				(*par)[2] = limit;
			}
		}
	}
	fclose(fd);
	if (verbose) {
		printf("-------------------------------------------------------------------------\n");
		printf("Summary:   ");
	}
	if (xmlout)
		printf("<memory><lowmem><utilization>%.3f</utilization><commit>%.3f</commit></lowmem>"
		       "<ram>%.3f</ram>"
		       "<memswap><utilization>%.3f</utilization><commit>%.3f</commit></memswap>"
		       "<allocmem><utilization>%.3f</utilization><commit>%.3f</commit><limit>%.3f</limit></allocmem></memory>",
			rutotal_utl.low_mem,
			rutotal_comm.low_mem,
			rutotal_utl.total_ram,
			rutotal_utl.mem_swap,
			rutotal_comm.mem_swap,
			rutotal_utl.alloc_mem,
			rutotal_comm.alloc_mem,
			rutotal_comm.alloc_mem_lim);
	else
		printf("%7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f\n",
			rutotal_utl.low_mem,
			rutotal_comm.low_mem,
			rutotal_utl.total_ram,
			rutotal_utl.mem_swap,
			rutotal_comm.mem_swap,
			rutotal_utl.alloc_mem,
			rutotal_comm.alloc_mem,
			rutotal_comm.alloc_mem_lim);
	if (numerator) {
		lm = lowmem / (1024 * 1024);
		r = ram / (1024 * 1024);
		rs = (ram + swap) / (1024 * 1024);
		if (verbose)
			 printf("        ");
		printf("%7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f\n",
			lm, lm, r, rs, rs, rs, rs, rs);
	}
	return ret;
}

int main(int argc, char **argv)
{
	struct CParam param;
	int ret, opt, verbose = 0, numerator = 0, xmlout = 0;

	while ((opt = getopt(argc, argv, "Avhx")) > 0) {
		switch(opt) {
		case 'v':
			verbose = 1;
			break;
		case 'A':
			numerator = 1;
			break;
		case 'x':
			xmlout = 1;
			break;
		case 'h':
			usage();
			exit(1);
			break;
		}
	}
	memset(&param, 0, sizeof(struct CParam));
	gparam = &param;
	get_pagesize();
	ret = calculate(numerator, verbose, xmlout);
	exit(ret);
}
