#include <stdio.h>
#include <libconfig.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#include "dag-task.h"
#include "dag-collapse.h"
#include "dag-candidate.h"
#include "taskset-create.h"

/**
 * global command line configuration
 */
static struct {
	int c_verbose;
	char* c_lname;
	char* c_oname;
	char* c_tname;
	int c_arb;	/**< arbitrary heuristic */
	int c_maxb;	/**< max benefit */
	int c_minp;	/**< min penalty */
} clc;

static const char* short_options = "hl:o:vt:abp";
static struct option long_options[] = {
    {"help",		no_argument, 		0, 'h'},
    {"log", 		required_argument, 	0, 'l'},
    {"output", 		required_argument, 	0, 'o'},
    {"verbose", 	no_argument, 		&clc.c_verbose, 1},
    {"task",		required_argument,	0, 't'},
    {"arbitrary",	no_argument,		0, 'a'},
    {"max-benefit",	no_argument,		0, 'b'},
    {"min-penalty",	no_argument,		0, 'p'},
    {0, 0, 0, 0}
};

static const char *usagec[] = {
"dts-cand-order: DAG Task Candidate List Ordered by Heuristics"
"Usage: dts-cand-order -t <TASK FILE> [OPTIONS]",
"OPTIONS:",
"	-h/-help		This message",
"	-l/-log <FILE>		Auditible log file",
"	-o/--output <FILE>	Output file",
"	-v/--verbose		Verbose output",
"REQUIRED:",
"	-t/--task-file		Task file generated by dts-gen-nodes",
"ONE OF:",
"	-a/--arbitrary		Arbitrary order",
"	-b/--max-benefit	Maximum benefit heuristic",
"	-p/--min-penalty	Minimum penalty heuristic",
"",
"OPERATION:",
"	dts-collapse produces the list of candidates per object",
"",
"EXAMPLES:",
"	# Order candidates of dtask.dot by max benefit",
"	> dts-cand-order -t dtask.dot -o dtask.cands -b",
};

void
usage() {
	for (int i = 0; i < sizeof(usagec) / sizeof(usagec[0]); i++) {
		printf("%s\n", usagec[i]);
	}
}

int
main(int argc, char** argv) {
	FILE *ofile = stdout;
	gsl_rng *r = NULL;
	dtask_t *task = NULL;
	int rv = -1; /* Assume failure */

	/*
	 * Initializer for the GNU Scientific Library for random numbers
	 * Suggested values for environment variables
	 *   GSL_RNG_TYPE=ranlxs2
	 *   GSL_RNG_SEED=`date +%s`
	 */
	ges_stfu();
	
	/* Parse those arguments! */
	while(1) {
		int opt_idx = 0;
		int c = getopt_long(argc, argv, short_options,
		    long_options, &opt_idx);
		if (c == -1) {
			break;
		}

		switch(c) {
		case 0:
			break;
		case 'h':
			usage();
			goto bail;
		case 'l':
			/* Needs to be implemented */
			printf("Log file not implemented\n");
			usage();
			goto bail;
		case 'o':
			clc.c_oname = strdup(optarg);
			break;
		case 't':
			clc.c_tname = strdup(optarg);
			break;
		case 'v':
			clc.c_verbose = 1;
			break;
		case 'a':
			clc.c_arb = 1;
			break;
		case 'b':
			clc.c_maxb = 1;
			break;
		case 'p':
			clc.c_minp = 1;
			break;
		default:
			printf("Unknown option %c\n", c);
			usage();
			goto bail;
		}
	}

	if (!clc.c_tname) {
		fprintf(stderr, "--task-file is a required option\n");
		usage();
		goto bail;
	}
	if (clc.c_oname) {
		ofile = fopen(clc.c_oname, "w");
		if (!ofile) {
			fprintf(stderr, "Unable to open %s for writing\n",
			    clc.c_oname);
			ofile = stdout;
			goto bail;
		}
	}
	if (!clc.c_arb && !clc.c_maxb && !clc.c_minp) {
		fprintf(stderr, "An ordering heuristic is required\n");
		usage();
		goto bail;
	}

	/* Read the task file */
	task = dtask_read_path(clc.c_tname);
	if (!task) {
		fprintf(stderr, "Unable to read file %s\n", clc.c_tname);
		goto bail;
	}

	cand_list_t *cand_list;
	if (clc.c_arb) {
		cand_list = corder_arb(task);
	}
	if (clc.c_maxb) {
		cand_list = corder_maxb(task);
	}
	if (clc.c_minp) {
		cand_list = corder_minp(task);
	}
	cand_t *cand = NULL;
	for(cand = cand_first(cand_list); cand; cand = cand_next(cand)) {
		fprintf(ofile, "%s %s\n", cand->c_a->dn_name,
			cand->c_b->dn_name);
	}

	cand_list_destroy(cand_list);
	
	rv = 0;
bail:
	if (task) {
		dtask_free(task);
	}
	if (clc.c_oname) {
		free(clc.c_oname);
	}
	if (clc.c_tname) {
		free(clc.c_tname);
	}
	if (r) {
		gsl_rng_free(r);
	}
	if (ofile != stdout) {
		fclose(ofile);
	}
	return rv;
}