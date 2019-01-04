#ifndef TASKSET_H
#define TASKSET_H

#include "task.h"
#include "search.h"

/**
 * Doubly Linked List wrapping the tasks
 */
typedef struct task_link {
	struct task_link *tl_next;
	struct task_link *tl_prev;
	task_t *tl_task;
} task_link_t;

typedef struct {
	task_link_t *ts_head;
} task_set_t;

task_set_t* ts_alloc();
void ts_free(task_set_t* ts);

char* ts_string(task_set_t *ts);


/**
 * Adding and removing tasks from the task set.
 *
 * When a task is added, a cookie is produced. The cookie is valid
 * until the next call of tset_*. 
 *
 * Usage:
 *
 *   task_link_t *cookie = tset_add(task); // task is now in the set
 *   // Time passes
 *   cookie = tset_find(task);
 *   tset_rem(cookie); // task is no longer in the set
 *   task_free(task);
 */

/**
 * Adds a task to the set
 *
 * @param[in] task the task being added to the set
 *
 * @return a cookie tracking where the task has been added in the set
 */
task_link_t* ts_add(task_set_t *ts, task_t *task);

/**
 * Finds a task in the set
 *
 * @param[in] task the task to search for
 *
 * @return a cookie if the task is in the set, NULL otherwise
 */
task_link_t* ts_find(task_set_t *ts, task_t *task);

/**
 * Removes a task from the set
 *
 * @param[in] cookie the cookie of the task in the set
 *
 * @return the task if the cookie was valid and the task was in the
 * set, NULL otherwise
 */
task_t* ts_rem(task_set_t *ts, task_link_t *cookie);

#endif /* TASKSET_H */
