#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "vzctl.h"
#include "validate.h"
#include "vzerror.h"
#include "config.h"
#include "util.h"

static long _PAGE_SIZE = 4096;

int getactionid(char *mode)
{
	if (mode == NULL)
		return ACT_NONE;
	if (!strcmp(mode, STR_ACT_NONE))
		return ACT_NONE;
	else if (!strcmp(mode, STR_ACT_WARN))
		return ACT_WARN;
	else if (!strcmp(mode, STR_ACT_ERROR))
		return ACT_ERROR;
	else if (!strcmp(mode, STR_ACT_FIX))
		return ACT_FIX;

	return ACT_NONE;
}

int validate_ve(int veid, struct CParam *param)
{
	int id;
	int ret;

	id = getactionid(param->validatemode);
	if (id == ACT_NONE)
		return 0;
	logger(0, 0, "Validate the Container:");
	ret  = validate(veid, param, id == ACT_FIX, 0);
	if (id == ACT_ERROR && ret)
		return VZ_VALIDATE_ERROR;
	return 0;
}

int read_yn(void)
{
	char buf[1024];

	fprintf(stderr, " (y/n) [y] ");
	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		if (buf[0] == 'y' || buf[0] == '\n') {
			return 1;
		}
		else if (buf[0] == 'n')
			return 0;
		fprintf(stderr, " (y/n) [y] ");
	}
	return 0;
}

int check_param(struct CParam *param, int log)
{
	int ret = 0;
#define CHECKPARAM(name) { \
	if (param->name == NULL) { \
		if (log) \
			fprintf(stderr, "Error: parameter " #name \
				" not found\n"); \
		ret = 1; \
	} \
}

	CHECKPARAM(numproc);
	CHECKPARAM(numtcpsock);
	CHECKPARAM(numothersock);
	CHECKPARAM(oomguarpages)
	CHECKPARAM(vmguarpages);
	CHECKPARAM(kmemsize);
	CHECKPARAM(tcpsndbuf);
	CHECKPARAM(tcprcvbuf);
	CHECKPARAM(othersockbuf);
	CHECKPARAM(dgramrcvbuf);
	CHECKPARAM(privvmpages);
	CHECKPARAM(numfile);
	CHECKPARAM(dcachesize);
	CHECKPARAM(physpages)
	CHECKPARAM(numpty)

	return ret;
}

int validate(int veid, struct CParam *param, int recover, int ask)
{
	unsigned long avnumproc;
	int ret = 0;
	unsigned long val, val1;
	unsigned long tmp_val0, tmp_val1;
	int changed = 0;
	int mm_mode;

#define SET_MES(val)	logger(0, 0, "set to %lu", val);
#define SET2_MES(val1, val2) logger(0, 0,"set to %lu:%lu", val1, val2);

#define CHECK_BL(name) \
if (param->name != NULL) { \
	if (param->name[0] > param->name[1]) { \
		logger(-1, 0, "Error: Barrier must be less than or equal to the limit for " \
			#name " (currently, %lu:%lu)", \
			param->name[0], param->name[1]); \
		if (ask || recover) { \
			tmp_val1 = param->name[0]; \
			tmp_val0 = param->name[0]; \
			SET2_MES(tmp_val0, tmp_val1) \
			if (ask) recover = read_yn(); \
			if (recover) { \
				param->name[1] = tmp_val1; \
				changed++; \
			} \
		} \
		if (!recover) ret = 1; \
	} \
} else { \
	logger(-1, 0, "Error: Parameter "  #name " is not found\n"); \
	ret = 1; \
}

#define CHECK_B(name) \
if (param->name != NULL) { \
	if ((param->name[0] != param->name[1])) { \
		logger(-1, 0, "Error: Barrier must be equal to the limit for " \
			#name " (currently, %lu:%lu)", \
			param->name[0], param->name[1]); \
		if (ask || recover) { \
			tmp_val0 = max_ul(param->name[0], param->name[1]); \
			tmp_val1 = tmp_val0; \
			SET2_MES(tmp_val0, tmp_val1) \
			if (ask) recover = read_yn(); \
			if (recover) { \
				param->name[0] = tmp_val0; \
				param->name[1] = tmp_val1; \
				changed++; \
			} \
		} \
		if (!recover) ret = 1; \
	} \
} else { \
	logger(-1, 0, "Error: Parameter "  #name " is not found\n"); \
	ret = 1; \
}

	if (param == NULL)
		return 1;
	mm_mode = get_conf_mm_mode(param);

	if (mm_mode == MM_VSWAP) {
		if (param->physpages == NULL) {
			logger(-1, 0, "Error: PHYSPAGES parameter"
					" is not found");
			return 1;
		}
		CHECK_BL(physpages);
		CHECK_BL(swappages);
		return 0;
	} else if (mm_mode == MM_SLM) {
		if (param->slmmode == SLM_MODE_SLM) {
			if (param->slmmemorylimit == NULL) {
				logger(-1, 0, "Error: SLMMEMORYLIMIT parameter"
						" is not found");
				return 1;
			}
			return 0;
		}
		if (param->slmmode == SLM_MODE_ALL && param->slmmemorylimit != NULL)
			return 0;
	}
	if (check_param(param, 1))
		return 1;
	if (param->avnumproc != NULL)
		avnumproc = param->avnumproc[0];
	else
		avnumproc = param->numproc[0] / 2;
/*	1 Check barrier & limit	*/
	/* Primary */
	CHECK_B(numproc)
	CHECK_B(numtcpsock)
	CHECK_B(numothersock)
	if (param->vmguarpages != NULL) {
		if (param->vmguarpages[1] != LONG_MAX &&
		    param->vmguarpages[1] != INT_MAX)
		{
			logger(-1, 0, "Error: Limit must be equal to %lu for"
				" vmguarpages (currently, %lu)", LONG_MAX,
				param->vmguarpages[1]);
			if (ask || recover) {
				SET_MES((unsigned long) LONG_MAX);
				if (ask)
					recover = read_yn();
				if (recover) {
					param->vmguarpages[1] = LONG_MAX;
					changed++;
				}
			}
			if (!recover) ret = 1;
//			if (!ask) fprintf(stderr, "\n");
		}
	} else {
		logger(-1, 0, "Error: Parameter vmguarpages is not found\n");
		ret = 1;
	}
	/* Secondary */
	CHECK_BL(kmemsize)
	CHECK_BL(tcpsndbuf)
	CHECK_BL(tcprcvbuf)
	CHECK_BL(othersockbuf)
	CHECK_BL(dgramrcvbuf)
	if (param->oomguarpages != NULL) {
		if (param->oomguarpages[1] != LONG_MAX &&
		    param->oomguarpages[1] != INT_MAX) {
			logger(-1, 0, "Error: Limit must be equal to %lu for"
				" oomguarpages (currently, %lu)", LONG_MAX,
				param->oomguarpages[1]);
			if (ask || recover) {
				SET_MES((unsigned long) LONG_MAX);
				if (ask)
					recover = read_yn();
				if (recover) {
					param->oomguarpages[1] = LONG_MAX;
					changed++;
				}
			}
			if (!recover) ret = 1;
//			if (!ask) fprintf(stderr, "\n");
		}
	} else {
		logger(-1, 0, "Error: Parameter oomguarpages is not found\n");
		ret = 1;
	}
	CHECK_BL(privvmpages)
	/* Auxiliary */
	CHECK_BL(lockedpages)
	CHECK_B(shmpages)
	if (param->physpages != NULL) {
		if (param->physpages[0] != 0) {
			logger(-1, 0, "Error: Barrier must be equal to 0 for"
				" physpages (currently, %lu)",
				param->physpages[0]);
			if (ask || recover) {
				SET_MES((unsigned long) 0);
				if (ask)
					recover = read_yn();
				if (recover) {
					param->physpages[0] = 0;
					changed++;
				}
			}
			if (!recover) ret = 1;
//			if (!ask) fprintf(stderr, "\n");
		}
		if (param->physpages[1] != LONG_MAX &&
		    param->physpages[1] != INT_MAX) {
			logger(-1, 0, "Error: Limit must be equal to %lu for"
				" physpages (currently, %lu)", LONG_MAX,
				param->physpages[1]);
			if (ask || recover) {
				SET_MES((unsigned long) LONG_MAX);
				if (ask)
					recover = read_yn();
				if (recover) {
					param->physpages[1] = LONG_MAX;
					changed++;
				}
			}
			if (!recover) ret = 1;
//			if (!ask) fprintf(stderr, "\n");
		}
	} else {
		logger(-1, 0, "Error: Parameter physpages is not found\n");
		ret = 1;
	}
	CHECK_B(numfile)
	CHECK_BL(numflock)
	CHECK_B(numpty)
	CHECK_B(numsiginfo)
	CHECK_BL(dcachesize)
	CHECK_B(numiptent)
	CHECK_BL(quota_block)
	CHECK_BL(quota_inode)

/*	2 Check formulas			*/
	if (param->privvmpages[0] < param->vmguarpages[0]) {
		logger(-1, 0, "Warning: privvmpages.bar must be greater than %lu"
				" (currently, %lu)", param->vmguarpages[0],
				param->privvmpages[0]);
		if (ask || recover) {
			tmp_val0 = param->vmguarpages[0];
			tmp_val1 = param->privvmpages[1] < tmp_val0 ?
				tmp_val0 : param->vmguarpages[1];
			SET_MES(tmp_val0);
			if (ask)
				recover = read_yn();
			if (recover) {
				param->privvmpages[0] = tmp_val0;
				param->privvmpages[1] = tmp_val1;
				changed++;
			}
		}
		if (!recover) ret = 1;
//		if (!ask) fprintf(stderr, "\n");
	}
	val = 2.5 * 1024 * param->numtcpsock[0];
	val &= LONG_MAX;
	if (param->tcpsndbuf[1] - param->tcpsndbuf[0] < val) {
		logger(-1, 0, "Error: tcpsndbuf.lim-tcpsndbuf.bar"
				" must be greater than %lu (currently, %lu)", val,
				param->tcpsndbuf[1]-param->tcpsndbuf[0]);
		if (ask || recover) {
			tmp_val1 = param->tcpsndbuf[0] + val;
			tmp_val0 = param->tcpsndbuf[0];
			SET2_MES(tmp_val0, tmp_val1);
			if (ask)
				recover = read_yn();
			if (recover) {
				param->tcpsndbuf[1] = tmp_val1;
				changed++;
			}
		}
		if (!recover) ret = 1;
//		if (!ask) fprintf(stderr, "\n");
	}
	val = 2.5 * 1024 * param->numothersock[0];
	val &= LONG_MAX;
	if ((param->othersockbuf[1] - param->othersockbuf[0]) < val) {
		logger(-1, 0, "Error: othersockbuf.lim-othersockbuf.bar"
			" must be greater than %lu (currently, %lu)", val,
			(param->othersockbuf[1]-param->othersockbuf[0]));
		if (ask || recover) {
			tmp_val1 = param->othersockbuf[0] + val;
			tmp_val0 = param->othersockbuf[0];
			SET2_MES(tmp_val0, tmp_val1)
			if (ask)
				recover = read_yn();
			if (recover) {
				param->othersockbuf[1] = tmp_val1;
				changed++;
			}
		}
		if (!recover) ret = 1;
//		if (!ask) fprintf(stderr, "\n");
	}
	val = 2.5 * 1024 * param->numtcpsock[0];
	val &= LONG_MAX;
	if ((param->tcprcvbuf[1] - param->tcprcvbuf[0]) < val)
	{
		logger(-1, 0, "Warning: tcprcvbuf.lim-tcprcvbuf.bar"
			" must be greater than %lu (currently, %lu)", val,
				param->tcprcvbuf[1] - param->tcprcvbuf[0]);
		if (ask || recover) {
			tmp_val1 = param->tcprcvbuf[0] + val;
			tmp_val0 = param->tcprcvbuf[0];
			SET2_MES(tmp_val0, tmp_val1)
			if (ask)
				recover = read_yn();
			if (recover) {
				param->tcprcvbuf[1] = tmp_val1;
				changed++;
			}
		}
		if (!recover) ret = 1;
//		if (!ask) fprintf(stderr, "\n");
	}
	val = 64 * 1024;
	if (param->tcprcvbuf[0] < val) {
		logger(-1, 0, "Warning: tcprcvbuf.bar must be greater than %lu\n"
				" (currently, %lu)", val,
				param->tcprcvbuf[0]);
		if (ask || recover) {
			tmp_val1 = param->tcprcvbuf[1]+val-param->tcprcvbuf[0];
			tmp_val0 = val;
			SET2_MES(tmp_val0, tmp_val1)
			if (ask)
				recover = read_yn();
			if (recover) {
				param->tcprcvbuf[1] = tmp_val1;
				param->tcprcvbuf[0] = tmp_val0;
				changed++;
			}
		}
		if (!recover) ret = 1;
//		if (!ask) fprintf(stderr, "\n");
	}
	val = 64 * 1024;
	if (param->tcpsndbuf[0] <  val) {
		logger(-1, 0, "Warning: tcpsndbuf.bar must be greater than %lu"
				" (currently, %lu)", val,
				param->tcpsndbuf[0]);
		if (ask || recover) {
			tmp_val1 = param->tcpsndbuf[1]+val-param->tcpsndbuf[0];
			tmp_val0 = val;
			SET2_MES(tmp_val0, tmp_val1)
			if (ask)
				recover = read_yn();
			if (recover) {
				param->tcpsndbuf[1] = tmp_val1;
				param->tcpsndbuf[0] = tmp_val0;
				changed++;
			}
		}
		if (!recover) ret = 1;
//		if (!ask) fprintf(stderr, "\n");
	}
	val = 32 * 1024;
	val1 = 129 * 1024;
	if (param->dgramrcvbuf[0] < val) {
		logger(-1, 0, "Warning: dgramrcvbuf.bar must be greater than"
				" %lu (currently, %lu)", val,
				param->dgramrcvbuf[0]);
		if (ask || recover) {
			tmp_val1 = param->dgramrcvbuf[1] + val -
					param->dgramrcvbuf[0];
			tmp_val0 = val;
			SET2_MES(tmp_val0, tmp_val1)
			if (ask)
				recover = read_yn();
			if (recover) {
				param->dgramrcvbuf[1] = tmp_val1;
				param->dgramrcvbuf[0] = tmp_val0;
				changed++;
			}
		}
		if (!recover) ret = 1;
//		if (!ask) fprintf(stderr, "\n");
	} else if (param->dgramrcvbuf[0] < val1) {
		logger(-1, 0, "Recommendation: dgramrcvbuf.bar must be greater than"
				" %lu (currently, %lu)", val1,
				param->dgramrcvbuf[0]);
		if (ask || recover) {
			tmp_val1 = param->dgramrcvbuf[1] + val1 -
					param->dgramrcvbuf[0];
			tmp_val0 = val1;
			SET2_MES(tmp_val0, tmp_val1)
			if (ask)
				recover = read_yn();
			if (recover) {
				param->dgramrcvbuf[1] = tmp_val1;
				param->dgramrcvbuf[0] = tmp_val0;
				changed++;
			}
		}
//		if (!ask) fprintf(stderr, "\n");
	}
	val =  32 * 1024;
	val1 = 129 * 1024;
	if (param->othersockbuf[0] < val) {
		logger(-1, 0, "Warning: othersockbuf.bar must be greater than"
				" %lu (currently, %lu)", val,
				param->othersockbuf[0]);
		if (ask || recover) {
			tmp_val1 = param->othersockbuf[1] + val -
					param->othersockbuf[0];
			tmp_val0 = val;
			SET2_MES(tmp_val0, tmp_val1)
			if (ask)
				recover = read_yn();
			if (recover) {
				param->othersockbuf[1] = tmp_val1;
				param->othersockbuf[0] = tmp_val0;
				changed++;
			}
		}
		if (!recover) ret = 1;
//		if (!ask) fprintf(stderr, "\n");
	} else if (param->othersockbuf[0] < val1) {
		logger(-1, 0, "Recommendation: othersockbuf.bar must be greater than"
				" %lu (currently, %lu)", val1,
				param->othersockbuf[0]);
		if (ask || recover) {
			tmp_val1 = param->othersockbuf[1] + val1 -
					param->othersockbuf[0];
			tmp_val0 = val1;
			SET2_MES(tmp_val0, tmp_val1)
			if (ask)
				recover = read_yn();
			if (recover) {
				param->othersockbuf[1] = tmp_val1;
				param->othersockbuf[0] = tmp_val0;
				changed++;
			}
		}
//		if (!ask) fprintf(stderr, "\n");
	}
	val = avnumproc * 32;
	val1 = param->numtcpsock[0] + param->numothersock[0] + param->numpty[0];
	if (val1 > val)
		val = val1;
	val &= LONG_MAX;
	if (param->numfile[0] < val) {
		logger(-1, 0, "Warning: numfile must be greater than %lu"
				" (currently, %lu)", val, param->numfile[0]);
		if (ask || recover) {
			tmp_val1 = param->numfile[1] + val - param->numfile[0];
			tmp_val0 = val;
			SET2_MES(tmp_val0, tmp_val1)
			if (ask)
				recover = read_yn();
			if (recover) {
				param->numfile[1] = tmp_val1;
				param->numfile[0] = tmp_val0;
				changed++;
			}
		}
		if (!recover) ret = 1;
//		if (!ask) fprintf(stderr, "\n");
	}
	val = param->numfile[0] * 384;
	val &= LONG_MAX;
	if (param->dcachesize[1] < val) {
		logger(-1, 0, "Warning: dcachesize.lim must be greater than %lu"
				" (currently, %lu)", val,
				param->dcachesize[1]);
		if (ask || recover) {
			SET_MES(val);
			if (ask)
				recover = read_yn();
			if (recover) {
				param->dcachesize[1] = val;
				changed++;
			}
		}
		if (!recover) ret = 1;
//		if (!ask) fprintf(stderr, "\n");
	}
	val = (40 * 1024 * avnumproc) + param->dcachesize[1];
	val &= LONG_MAX;
	if (param->kmemsize[0] < val) {
		logger(-1, 0, "Error: kmemsize.bar must be greater than %lu"
				" (currently, %lu)", val, param->kmemsize[0]);
		if (ask || recover) {
			tmp_val1 = param->kmemsize[1] + val- param->kmemsize[0];
			tmp_val0 = val;
			SET2_MES(tmp_val0, tmp_val1);
			if (ask)
				recover = read_yn();
			if (recover) {
				param->kmemsize[1] = tmp_val1;
				param->kmemsize[0] = tmp_val0;
				changed++;
			}
		}
		if (!recover) ret = 1;
//		if (!ask) fprintf(stderr, "\n");
	}
	if (recover || ask) {
		if (changed &&
			param->class_id != NULL &&
			*param->class_id == 1)
		{
			if (ask) {
				fprintf(stderr, "Change Container class from 1 to 2");
				if (read_yn()) {
					*param->class_id = 2;
				}
			} else {
				fprintf(stderr, "Container class changed from 1 to 2\n");
				*param->class_id = 2;
			}
		}
	}
	return ret;
}

struct CRusage * calc_ve_utilization(struct CParam *param, int numerator)
{
	struct CRusage *rusage;
	double kmem_net;
	unsigned long long lowmem;
	unsigned long long ram, swap = 1;

	if (param == NULL)
		return NULL;
	if (get_mem(&ram))
		return NULL;
	get_swap(&swap);
	if (get_lowmem(&lowmem))
		return NULL;
	lowmem *= 0.4;
	if (check_param(param, 1))
		return NULL;
	rusage = (struct CRusage *) malloc(sizeof(struct CRusage));
	memset(rusage, 0, sizeof(struct CRusage));
	kmem_net = (double)param->kmemsize[0] +
		(double)param->tcprcvbuf[0] +
		(double)param->tcpsndbuf[0] +
		(double)param->dgramrcvbuf[0] +
		(double)param->othersockbuf[0];
	/*	Low memory	*/
	rusage->low_mem = kmem_net;
	if (!numerator)
		rusage->low_mem /= lowmem;
	/*	Total RAM	*/
	rusage->total_ram = ((double)param->physpages[0] * _PAGE_SIZE);
	if (!numerator)
		rusage->total_ram /= ram;
	/*	Mem + Swap	*/
	rusage->mem_swap = ((double)param->oomguarpages[0] * _PAGE_SIZE);
	if (!numerator)
		rusage->mem_swap /= (ram + swap);
	/*	Allocated memory	*/
	rusage->alloc_mem = ((double)param->privvmpages[0] * _PAGE_SIZE);
	if (!numerator)
		rusage->alloc_mem /= (ram + swap);

	return rusage;
}

static double get_res_val(unsigned long *res, int idx)
{
	return (res == NULL ? LONG_MAX : res[idx]) * _PAGE_SIZE;
}

struct CRusage * calc_ubc_ve_commitment(struct CParam *param, int numerator)
{
	struct CRusage *rusage;
	unsigned long long ram, swap = 1;

	if (param== NULL)
		return NULL;
	if (get_mem(&ram))
		return NULL;
	get_swap(&swap);
	rusage = (struct CRusage *) calloc(1, sizeof(struct CRusage));
	/*      Total RAM       */
	rusage->total_ram = get_res_val(param->physpages, 0);
	if (!numerator)
		rusage->total_ram /= ram;
	/*      Low memory      */
	rusage->low_mem = rusage->total_ram;
	/*	Mem + Swap	*/
	rusage->mem_swap = get_res_val(param->oomguarpages, 0);
	if (!numerator)
		rusage->mem_swap /= (ram + swap);
	/*	Allocated memory	*/
	rusage->alloc_mem = get_res_val(param->vmguarpages, 0);
	if (!numerator)
		rusage->alloc_mem /= (ram + swap);
	/*	Allocated memory limit	*/
	rusage->alloc_mem_lim = get_res_val(
			(param->privvmpages == NULL || param->privvmpages[1] == LONG_MAX) ?
			param->physpages : param->privvmpages, 1);
	if (!numerator)
		rusage->alloc_mem_lim /= (ram + swap);
	/*	Max Allocated memory limit	*/
	rusage->alloc_mem_max_lim = get_res_val(
			(param->privvmpages == NULL || param->privvmpages[1] == LONG_MAX) ?
			param->physpages : param->privvmpages, 1);
	if (!numerator)
		rusage->alloc_mem_max_lim /= (ram + swap);
	return rusage;
}

struct CRusage * calc_ve_commitment(int veid, struct CParam *param,
	int numerator)
{
	return calc_ubc_ve_commitment(param, numerator);
}

inline int inc_rusage(struct CRusage *rusagetotal, struct CRusage *rusageve)
{
	if (rusagetotal == NULL || rusageve == NULL)
		return 1;
	rusagetotal->low_mem += rusageve->low_mem;
	rusagetotal->total_ram += rusageve->total_ram;
	rusagetotal->mem_swap += rusageve->mem_swap;
	rusagetotal->alloc_mem += rusageve->alloc_mem;
	rusagetotal->alloc_mem_lim += rusageve->alloc_mem_lim;
	if (rusageve->alloc_mem_max_lim > rusagetotal->alloc_mem_max_lim)
		rusagetotal->alloc_mem_max_lim = rusageve->alloc_mem_max_lim;

	return 0;
}

inline int mul_rusage(struct CRusage *rusage, int k)
{
	if (rusage == NULL)
		return 1;
	rusage->low_mem *= k;
	rusage->total_ram *= k;
	rusage->mem_swap *= k;
	rusage->alloc_mem *= k;
	rusage->alloc_mem_lim *= k;
	rusage->alloc_mem_max_lim *= k;

	return 0;
}

struct CRusage *calc_hn_rusage(int mode)
{
	FILE *fd;
	struct CRusage *rusagetotal, *rusage = NULL;
	char str[STR_SIZE];
	char name[STR_SIZE];
	const char *fmt;
	unsigned long held, maxheld, barrier, limit;
	int found = 0;
	int id, res, i;
	int veid = 0;
	int *veids = NULL, numve;

	if ((fd = fopen(PROCUBC, "r")) == NULL) {
		logger(-1, errno, "Unable to open " PROCUBC);
		return NULL;
	}
	numve = get_run_ve(&veids);
	rusagetotal = (struct CRusage *) malloc(sizeof (struct CRusage));
	memset(rusagetotal, 0, sizeof(struct CRusage));
	memset(&tmpparam, 0, sizeof(struct CParam));
	while (fgets(str, sizeof(str), fd)) {
		if ((res = sscanf(str, "%d:", &id)) == 1) {
			fmt =  "%*lu:%s%lu%lu%lu%lu";
			found = 1;
			if (veid && find_veid(veid, veids, numve) != NULL) {
				if (mode == UTILIZATION)
					rusage = calc_ve_utilization(&tmpparam, 0);
				else
					rusage = calc_ve_commitment(veid, &tmpparam, 0);
				inc_rusage(rusagetotal, rusage);
				free(rusage);
				rusage = NULL;
			}
			memset(&tmpparam, 0, sizeof(struct CParam));
			veid = id;
		} else {
			fmt = "%s%lu%lu%lu%lu";
		}
		if (found) {
			if ((res = sscanf(str, fmt, name, &held, &maxheld,
					&barrier, &limit)) == 5)
			{
				for (i = 0; name[i] != 0; i++)
					name[i] = toupper(name[i]);
				if ((i = FindName(name)) >= 0) {
					unsigned long **par = Config[i].pvar;
					*par = (unsigned long *)malloc
						(sizeof(unsigned long) * 2);
					if (mode == UTILIZATION) {
						(*par)[0] = held;
						(*par)[1] = held;
					} else {
						(*par)[0] = barrier;
						(*par)[1] = limit;
					}
				}
			}
		}
	}
	/* Last Container in /proc/user_beancounters */
	if (veid && find_veid(veid, veids, numve) != NULL) {
		if (mode == UTILIZATION)
			rusage = calc_ve_utilization(&tmpparam, 0);
		else
			rusage = calc_ve_commitment(veid, &tmpparam, 0);
		inc_rusage(rusagetotal, rusage);
		free(rusage);
	}
	fclose(fd);
	return rusagetotal;
}

int calc_hn_cpuusage(void)
{
	FILE *fd;
	char str[STR_SIZE];
	int weight = 0;
	int id[6];
	int res;

	if (!(fd = fopen(PROCFSHD, "r")))
		return 0;
	while (fgets(str, sizeof(str), fd))
	{
		res = sscanf(str, "%d%d%d%d%d%d", &id[0], &id[1], &id[2],
			&id[3], &id[4], &id[5]);
		if (res == 6 && id[0] == 0 && id[4] == 0)
		{
			weight += id[5];
		}
	}
	return weight;
}
/*
struct CRusage *calc_hn_rusage()
{
	struct CRusage *rusagetotal, *rusage = NULL;
	struct CList *ves, *l;
	char buf[STR_SIZE]

	ves = get_run_ve();
	rusagetotal = (struct CRusage *) malloc(sizeof (struct CRusage));
	memset(rusagetotal, 0, sizeof(struct CRusage));
	for (l = ves; l != NULL; l = l->next)
	{
		snprintf(buf, sizeof(buf), "");
		ParseConfig(buf, param);
		rusage = calc_ve_overcommitment(param);
		inc_rusage(rusagetotal, rusage);
		FreeParam(param);
	}
	ListFree(ves);
	return rusagetotal;
}
*/
int check_hn_overcommitment(int veid, struct CParam *param)
{
	struct CRusage *rusage, *rusage_ve;
	int actid;
	int ret = 0;

	if (param == NULL || param->ovcaction == NULL)
		return 0;
	actid = getactionid(param->ovcaction);
	if (actid == ACT_NONE)
		return 0;
	/* Calculate current HN overcommitment */
	if ((rusage = calc_hn_rusage(COMMITMENT)) == NULL)
		return 0;
	/* Add Container resource usage */
	rusage_ve = calc_ve_commitment(veid, param, 0);
	inc_rusage(rusage, rusage_ve);
	/* Convert to % */
	mul_rusage(rusage, 100);
	free(rusage_ve);
	if (param->ovclevel_low_mem != NULL &&
			rusage->low_mem > *param->ovclevel_low_mem)
	{
		logger(0, 0, "%s: System is overcommitted.",
				actid == ACT_ERROR ? "Error" : "Warning");
		logger(0, 0, "\tLow Memory commitment level: (%.3f%%)"
				" exceeds the configured value (%.3f%%)",
				rusage->low_mem, *param->ovclevel_low_mem);
		ret = 1;
	}
	if (param->ovclevel_mem_swap != NULL &&
			rusage->mem_swap > *param->ovclevel_mem_swap)
	{
		if (!ret)
			logger(0, 0, "%s: System is overcommitted.",
				actid == ACT_ERROR ? "Error" : "Warning");
		logger(0, 0, "\tMemory and Swap commitment level: (%.3f%%)"
				" exceeds the configured value (%.3f%%)",
				rusage->mem_swap, *param->ovclevel_mem_swap);
		ret = 1;
	}
	if (param->ovclevel_alloc_mem != NULL &&
			rusage->alloc_mem > *param->ovclevel_alloc_mem)
	{
		if (!ret)
			logger(0, 0, "%s: System is overcommitted.",
				actid == ACT_ERROR ? "Error" : "Warning");
		logger(0, 0, "\tAllocated Memory commitment level: (%.3f%%)"
				" exceeds the configured value (%.3f%%)",
				rusage->alloc_mem, *param->ovclevel_alloc_mem);
		ret = 1;
	}
	if (param->ovclevel_alloc_mem_lim != NULL &&
			rusage->alloc_mem_lim > *param->ovclevel_alloc_mem_lim)
	{
		if (!ret)
			logger(0, 0, "%s: System is overcommitted.",
				actid == ACT_ERROR ? "Error" : "Warning");
		logger(0, 0, "\tTotal Alloc Limit commitment level (%.3f%%)"
				" exceeds the configured value (%.3f%%)",
				rusage->alloc_mem_lim, *param->ovclevel_alloc_mem_lim);
		ret = 1;
	}
	free(rusage);
	return actid == ACT_ERROR ? VZ_OVERCOMMIT_ERROR : 0;
}
