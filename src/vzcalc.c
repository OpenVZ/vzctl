#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

#include "vzctl.h"
#include "validate.h"
#include "config.h"
#include "util.h"

struct CParam *gparam, *lparam;
int get_thrmax(int *thrmax);

void usage(void)
{
	printf("Usage: vzcalc [-v] <ctid>\n");
}

int update_vswap_param(int veid, struct CParam *vswap_param);

int calculate(int veid, struct CParam *param, int verbose)
{
	struct CParam param_cur;
	struct CRusage *ru_utl = NULL, *ru_comm = NULL;
	char tmp[STR_SIZE];
	int ret = 0;
	double lm_cur, tr_cur, ms_cur, al_cur, np_cur;
	double lm_max, lm_prm, ms_prm, al_prm, al_max, np_max;
	double cur, prm, max;
	unsigned long long ram, swap = 1;
	int run, thrmax;

	if (param == NULL)
		return 1;
	if (get_mem(&ram))
		return 1;
	if (get_thrmax(&thrmax))
		return 1;
	get_swap(&swap);
	lm_cur = tr_cur = ms_cur = al_cur = np_cur = 0;
	if (!(run = get_ubc(veid, &param_cur))) {
		if (NULL == (ru_utl = calc_ve_utilization(&param_cur, 0)))
			ru_utl = (struct CRusage *)calloc(1, sizeof(struct CRusage));

		/*	Low memory	*/
		lm_cur = ru_utl->low_mem;
		/*	Total RAM	*/
		tr_cur = ru_utl->total_ram;
		/*	Mem + Swap	*/
		ms_cur = ru_utl->mem_swap;
		/*	Allocated	*/
		al_cur = ru_utl->alloc_mem;
		/*	Numproc		*/
		np_cur = (double)param_cur.numproc[0] / thrmax;
	}

	update_vswap_param(veid, param);
	if ( NULL == (ru_comm = calc_ve_commitment(veid, param, 0)))
		ru_comm = (struct CRusage *)calloc(1, sizeof(struct CRusage));
	/*      Low memory      */
	lm_max = ru_comm->low_mem;
	lm_prm = lm_max;
	/*      Mem + Swap      */
	ms_prm = ru_comm->mem_swap;
	/*      Allocated       */
	al_prm = ru_comm->alloc_mem;
	al_max = ru_comm->alloc_mem_lim;
	/*	Numproc		*/
	np_max = param->numproc?(double)param->numproc[0] / thrmax : 0;

	/* Calculate maximum for current */
	cur = lm_cur;
	if (tr_cur > cur)
		cur = tr_cur;
	if (ms_cur > cur)
		cur = ms_cur;
	if (al_cur > cur)
		cur = al_cur;
	if (np_cur > cur)
		cur = np_cur;
	/* Calculate maximum for promised */
	prm = lm_prm;
	if (ms_prm > prm)
		prm = ms_prm;
	if (al_prm > prm)
		prm = al_prm;
	/* Calculate maximum for Max */
	max = lm_max;
	if (al_max > max)
		max = al_max;
	sprintf(tmp, "n/a");
	printf("Resource     Current(%%)  Promised(%%)  Max(%%)\n");
	if (verbose) {
		if (!run)
			sprintf(tmp, "%10.2f", lm_cur * 100);
		printf("Low Mem    %10s %10.2f %10.2f\n",
				tmp, lm_prm * 100, lm_max * 100);
		if (!run)
			sprintf(tmp, "%10.2f", tr_cur * 100);
		printf("Total RAM  %10s        n/a        n/a \n", tmp);
		if (!run)
			sprintf(tmp, "%10.2f", ms_cur * 100);
		printf("Mem + Swap %10s %10.2f        n/a\n",
				tmp, ms_prm * 100);
		if (!run)
			sprintf(tmp, "%10.2f", al_cur * 100);
		printf("Alloc. Mem %10s %10.2f %10.2f\n",
				tmp , al_prm * 100, al_max * 100);
		if (!run)
			sprintf(tmp, "%10.2f", np_cur * 100);
		printf("Num. Proc  %10s        n/a %10.2f\n",
				tmp, np_max * 100);
		printf("--------------------------------------------\n");
	}
	if (!run)
		sprintf(tmp, "%10.2f", cur * 100);
	printf("Memory     %10s %10.2f %10.2f\n", tmp, prm * 100, max * 100);
	return ret;
}

int main(int argc, char **argv)
{
	struct CParam param;
	struct stat st;
	char infile[STR_SIZE];
	int ret, opt, veid, verbose = 0;

	while ((opt = getopt(argc, argv, "vh")) > 0) {
		switch(opt) {
		case 'v':
			verbose = 1;
			break;
		case 'h':
			usage();
			exit(1);
			break;
		default	:
			exit(1);
		}
	}
	if (optind == argc) {
		usage();
		exit(1);
	}
	if (parse_int(argv[optind], &veid)) {
		fprintf(stderr, "Invalid veid: %s\n", argv[optind]);
		exit(1);
	}

	snprintf(infile, sizeof(infile), SCRIPT_DIR "/%d.conf", veid);
	if (stat(infile, &st)) {
		fprintf(stderr, "Container configuration file: %s not found\n",
				infile);
		exit(1);
	}
	memset(&param, 0, sizeof(struct CParam));
	gparam = &param;
	if (ParseConfig(veid, infile, &param, 0))
		exit(1);
	get_pagesize();
	ret = calculate(veid, &param, verbose);
	exit(ret);
}
