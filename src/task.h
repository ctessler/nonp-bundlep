#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TASK_NAMELEN 64

typedef struct {
	char t_name[TASK_NAMELEN];
	uint32_t t_period;
	uint32_t t_deadline;
	uint32_t t_threads;
	uint32_t t_chunk;	/**< Maximum non-preepmtive chunk (q) */
	uint32_t *t_wcet;
} task_t;

/**
 * WCET of n threads
 *
 * @param[in] the number of threads
 *
 * Usage:
 *     task_t t;
 *     t.wcet(1) = 25;
 *     t.wcet(2) = 35;
 *
 *     printf("WCET of 2 threads, %u\n", t.wcet(2));
 * 
 */
#define wcet(n) t_wcet[n-1]

/**
 * Allocates a new task
 *
 * @note a task must be task_free()'d 
 *
 * A task may be created with a peroid, deadline, and number of
 * threads equal to zero. 
 *
 * @param[in] period the minimum inter-arrival time of a job
 * @param[in] deadline the relative deadline
 * @param[in] threads the number of threads released with each job
 *
 * @return the new task
 */
task_t* task_alloc(uint32_t period, uint32_t deadline, uint32_t threads);
void task_free(task_t *task);

/**
 * Duplicates a task
 *
 * @param[in] orig the task being duplicated
 * @param[in] threads the number of threads in the new duplicate task
 */
task_t* task_dup(task_t *orig, uint32_t threads);

/**
 * Updates the number of threads a task releases with each job
 *
 * Usage:
 *     task_t *t = task_alloc(10, 20, 3);
 *     task_threads(t, 5);
 *     t->wcet(1) = 3;
 *     t->wcet(2) = 5;
 * 
 * @note changing the number of threads destroys the WCET table
 *
 * @param[in] threads the new number of threads released with each job
 *
 * @return the number of threads in the resized task
 */
int task_threads(task_t *task, uint32_t threads);

/**
 * Updates the name of the task
 *
 * @param[in] task the task
 * @param[in] name the name of the task
 */
void task_name(task_t *task, const char* name);

/**
 * Returns a dynamically allocated string representing the task
 *
 * @note the string must be free()'d
 */
char *task_string(task_t *task);

/**
 * Returns a dynamically allocated string to head the task
 *
 * @note the string must be free()'d
 */
char *task_header(task_t *task);

/**
 * Calculates the utilization for the task
 *
 * @param[in] task the task
 *
 * @return the utilization of the task
 */
float_t task_util(task_t *task);

/**
 * Calculates the maximum demand for the task over t time units
 *
 * @param[in] task the task
 * @param[in] t the units of time
 *
 * @return the maximum demand of task over t time units
 */
uint32_t task_dbf(task_t *task, uint32_t t);
uint32_t task_dbf_debug(task_t *task, uint32_t t, FILE *f);

#endif /* TASK_H */
