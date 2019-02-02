#include "taskset-deadlines.h"

int
ts_fill_deadlines(task_set_t *ts, ordl_t *head, uint32_t t) {
	task_link_t *cookie;
	task_t *task;
	for (cookie = ts_first(ts); cookie; cookie = cookie->tl_next) {
		task = ts_task(cookie);
		ts_fill_deadlines_task(task, head, t);
	}
	return 1;
}

int
ts_fill_deadlines_task(task_t *task, ordl_t *head, uint32_t t) {
	uint32_t deadline, i = 0;
	or_elem_t *D = NULL;
	do {
		deadline = task->t_deadline + (i * task->t_period);
		if (deadline <= t) {
			D = ordl_find(head, t);
			if (!D) {
				D = oe_alloc();
				D->oe_deadline = deadline;
			}
			D->oe_task = task;
			ts_add(D->oe_tasks, task);
			ordl_insert(head, D);
		}
		i++;
	} while (deadline <= t);

	return i;
}

int
ts_extend_deadlines(task_set_t *ts, ordl_t* head, uint32_t prevb,
    uint32_t newb) {
	if (prevb >= newb) {
		return 0;
	}
	task_link_t *cookie;
	task_t *task;
	for (cookie = ts_first(ts); cookie; cookie = ts_next(ts, cookie)) {
		task_t *task = ts_task(cookie);
		uint32_t deadline, i=0;
		do {
			deadline = task->t_deadline + (i * task->t_period);
			i++;
			if (deadline <= prevb) {
				continue;
			}
			if (deadline <= newb) {
				or_elem_t *D = oe_alloc();
				D->oe_deadline = deadline;
				D->oe_task = task;
				ordl_insert(head, D);
			}
		} while (deadline <= newb);
	}
}