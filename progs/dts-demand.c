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
	tint_t c_objs;
	tint_t c_wcet;
	float_t c_growf;
} clc;

static const char* short_options = "hl:o:vt:j:w:f:";
static struct option long_options[] = {
    {"help",		no_argument, 		0, 'h'},
    {"log", 		required_argument, 	0, 'l'},
    {"output", 		required_argument, 	0, 'o'},
    {"verbose", 	no_argument, 		&clc.c_verbose, 1},
    {"task",		required_argument,	0, 't'},
    {"objects",		required_argument,	0, 'j'},
    {"max-wcet",	required_argument,	0, 'w'},
    {"max-growf",	required_argument,	0, 'f'},
    {0, 0, 0, 0}
};

static const char *usagec[] = {
"dts-demand: DAG Task Assignment of Executable Objects and Demand",
"Usage: dts-demand -t <TASK FILE> [OPTIONS]",
"OPTIONS:",
"	-h/-help		This message",
"	-l/-log <FILE>		Auditible log file",
"	-o/--output <FILE>	Output file",
"	-v/--verbose		Verbose output",
"REQUIRED:",
"	-t/--task-file		Task file generated by dts-gen-nodes",
"	-j/--objects <INT>	Number of unique executable objects",
"	-w/--max-wcet <INT>	Maximum WCET (of one thread) of any object",
"	-f/--max-growf <FLOAT>	Maximum growth factor [.1, 1.0]",
"",
"OPERATION:",
"	dts-demand creates a pool of executable objects which are assigned to ",
"	individual nodes of the task. The max-wcet defines the WCET of each object",
"	for one thread, with a minimum of one cycle. Parameter max-growf is a ",
"	ceiling on the growth factor of	every node, with a minimum of 0.1",
"",
"EXAMPLES:"
"	# Create a task from task.dot with 10 objects, maximum WCET of 50",
"       # and maximum growth factor 0.5",
"	> dts-demand -t task.dot -j 10 -w 50 -f 0.5",
"",
"	# Write the augmented task to a new file (cannot be the same0",
"	> dts-demand -t task.dot -j 10 -w 50 -f 0.5 -o task-wcet.dot",
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
	FILE *tfile = NULL;
	gsl_rng *r = NULL;
	dtask_t *task = NULL;
	int rv = -1; /* Assume failure */
	tint_t *wcet; /* Arrays of objects wcet and growth factor */
	float_t *growf;

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
		case 'j': 
			clc.c_objs = atoi(optarg);
			break;
		case 'w':
			clc.c_wcet = atoi(optarg);
			break;
		case 'f':
			clc.c_growf = atof(optarg);
			break;
		case 'v':
			clc.c_verbose = 1;
			break;
		default:
			printf("Unknown option %c\n", c);
			usage();
			goto bail;
		}
	}

	if (clc.c_wcet <= 0) {
		fprintf(stderr, "--max-wcet is a required option\n");
		goto bail;
	}
	if (clc.c_growf > 1 || clc.c_growf <= 0.1) {
		fprintf(stderr, "--max-growf is a required option [0.1, 1]\n");
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
	tfile = fopen(clc.c_tname, "r");
	if (!tfile) {
		fprintf(stderr, "Unable to open %s for reading\n", clc.c_tname);
		goto bail;
	}
	task = dtask_read(tfile);
	if (!task) {
		fprintf(stderr, "Unable to read file %s\n", clc.c_tname);
		goto bail;
	}
	fclose(tfile);
	tfile = NULL;

	/* Initialize a random source */
	r = gsl_rng_alloc(gsl_rng_default);

	/* Allocate the objects WCET and growth factor */
	wcet = calloc(clc.c_objs, sizeof(tint_t));
	growf = calloc(clc.c_objs, sizeof(float_t));

	for (int i = 0; i < clc.c_objs; i++) {
		wcet[i] = tsc_get_scaled(r, 1, clc.c_wcet);
		growf[i] = tsc_get_scaled_dbl(r, 0.1, clc.c_growf);
	}

	/* Assign objects to nodes randomly */
	dnode_t *prev = NULL, *node = NULL;
	while (node = dtask_next_node(task, prev)) {
		if (prev) {
			dnode_free(prev);
		}
		prev = node;

		int objidx = tsc_get_scaled(r, 0, clc.c_objs - 1);
		dnode_set_threads(node, 1);
		dnode_set_object(node, objidx);
		dnode_set_wcet_one(node, wcet[objidx]);
		dnode_set_factor(node, growf[objidx]);
		dnode_update(node);
	}
	dnode_free(node);
	

	dtask_write(task, ofile);
	
	rv = 0;
bail:
	if (wcet) {
		free(wcet);
	}
	if (growf) {
		free(growf);
	}
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
	if (tfile) {
		fclose(tfile);
	}
	return rv;
}
