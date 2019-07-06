#include <stdio.h>
#include <libconfig.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#include "dag-task.h"
#include "taskset-create.h"

/**
 * global command line configuration
 */
static struct {
	int c_verbose;
	char* c_lname;
	char* c_oname;
	char* c_tname;
	tint_t c_period;
	float_t c_util;
} clc;

static const char* short_options = "hl:o:vt:p:u:";
static struct option long_options[] = {
    {"help",		no_argument, 		0, 'h'},
    {"log", 		required_argument, 	0, 'l'},
    {"output", 		required_argument, 	0, 'o'},
    {"verbose", 	no_argument, 		&clc.c_verbose, 1},
    {"task",		required_argument,	0, 't'},
    {"period",		required_argument,	0, 'p'},
    {"util",		required_argument,	0, 'u'},
    {0, 0, 0, 0}
};

static const char *usagec[] = {
"dts-period: DAG Task Assignment of Periods",
"Usage: dts-period -t <TASK FILE> -p|-u <VALUE> [OPTIONS]",
"OPTIONS:",
"	-h/-help		This message",
"	-l/-log <FILE>		Auditible log file",
"	-o/--output <FILE>	Output file",
"	-v/--verbose		Verbose output",
"REQUIRED:",
"	-t/--task-file		Task file generated by dts-gen-nodes",
"ONE OF:",
"	-p/--period <INT>	Period",
"	-u/--util <FLOAT> 	Utilization to infer period from",
"",
"OPERATION:",
"	dts-period creates a new DAG task from the given task with the period",
"	provided. If provided -p, the value is set. If provided by -u, the value is",
"	calculated using the workload of the task", 
"",
"EXAMPLES:"
"	# Create a task",
"	> dts-gen-nodes -n 20 -e 0.7 -o part-01.dot",
"	# Create demand",
"	> dts-demnad -t part-01.dot -j 10 -w 50 -f 0.3 -o part-02.dot",
"	# Assign period by utilization",
"	> dts-period -t part-02.dot -u 0.3 -o util.dot",
"",
"	# Assign period by value",
"	> dts-period -t part-02.dot -p 500",
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
		case 'p': 
			clc.c_period = atoi(optarg);
			break;
		case 'u':
			clc.c_util = atof(optarg);
			break;
		default:
			printf("Unknown option %c\n", c);
			usage();
			goto bail;
		}
	}

	if (clc.c_period <= 0 && clc.c_util <= 0) {
		fprintf(stderr, "A positive -u or -p is required\n");
		goto bail;
	}
	if (!clc.c_tname) {
		fprintf(stderr, "--taks-file is a required option\n");
	}
	if (clc.c_oname) {
		ofile = fopen(clc.c_oname, "w");
		if (!ofile) {
			fprintf(stderr, "Unable to open %s for writing\n", clc.c_oname);
			ofile = stdout;
			goto bail;
		}
	}

	/* Read the task file */
	task = dtask_read_path(clc.c_tname);
	if (!task) {
		fprintf(stderr, "Unable to read file %s\n", clc.c_tname);
		goto bail;
	}

	if (clc.c_period) {
		task->dt_period = clc.c_period;
	} else {
		/* Make sure all the values have been updated */
		tint_t work = dtask_workload(task);
		task->dt_period = ceil((double) work * clc.c_util);
	}
	dtask_update(task);
	dtask_write(task, ofile);
	
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
