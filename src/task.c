#include <task.h>
static char BUFF[1024];

task_t*
task_alloc(tint_t period, tint_t deadline, tint_t threads) {
	task_t* task = calloc(sizeof(task_t), 1);
	task_threads(task, threads);
	task->t_period = period;
	task->t_deadline = deadline;
	return task;
}

void
task_free(task_t* task) {
	if (!task) {
		return;
	}
	task_threads(task, 0);
	free(task);
}

task_t*
task_dup(task_t *orig, tint_t threads) {
	if (threads > orig->t_threads) {
		/* XXX-ct assert here */
		return NULL;
	}
	task_t *task = task_alloc(orig->t_period, orig->t_deadline, threads);
	for (int i=1; i <= threads; i++) {
		task->wcet(i) = orig->wcet(i);
	}
	strncpy(task->t_name, orig->t_name, TASK_NAMELEN);

	return task;
}

int
task_threads(task_t *task, tint_t threads) {
	if (task->t_wcet) {
		free(task->t_wcet);
		task->t_wcet = NULL;
	}
	task->t_threads = threads;
	if (threads > 0) {
		task->t_wcet = calloc(sizeof(tint_t), threads);
	}

	return threads;
}

void
task_name(task_t* task, const char* name) {
	strncpy(task->t_name, name, TASK_NAMELEN);
}

char *
task_string(task_t *task) {
	char *s = BUFF;
	int n = sprintf(BUFF, "(p:%4lu, d:%4lu, m:%2lu)",
	    task->t_period, task->t_deadline, task->t_threads);
	s += n;
	n = sprintf(s, " [u:%.3f, q:%lu, %s]\twcet{", task_util(task), task->t_chunk,
		task->t_name);
	for (int i=1; i <= task->t_threads; i++) {
		s += n;
		n = sprintf(s, "%3lu", task->wcet(i));
		if (i < task->t_threads) {
			s += n;
			n = sprintf(s, ", ");
		}
	}
	s += n;
	n = sprintf(s, "} ");
	s += n;
	return strdup(BUFF);
}

char *
task_header(task_t *task) {
	sprintf(BUFF,
	    "(period, dedlin, tpj.) [util(m) chunk name]\twcet{c(1), c(2), ..., c(m)}");
	return strdup(BUFF);
}


float_t
task_util(task_t *task) {
	tint_t m,c;
	m = task->t_threads;
	if (m <= 0) {
		return 0;
	}
	c = task->wcet(m);

	return (float_t)((float_t) c / task->t_period);
}

tint_t
task_dbf(task_t *task, tint_t t) {
	if (t < task->t_deadline) {
		return 0;
	}
	tint_t j = t - task->t_deadline;
	j = j / task->t_period;
	j += 1;
	j *= task->wcet(task->t_threads);

	return j;
}

tint_t
task_dbf_debug(task_t *task, tint_t t, FILE *f) {
	if (t < task->t_deadline) {
		fprintf(f, "DBF(%s, t = %lu) = 0 ", task->t_name, t);
		fprintf(f, ": t < deadline (%lu < %lu)\n", t, task->t_deadline);
		return 0;
	}
	tint_t numerator = t - task->t_deadline;
	tint_t denominator = task->t_period;
	tint_t wcet = task->wcet(task->t_threads);
	tint_t frac = numerator / denominator;
	tint_t demand = (frac + 1) * wcet;
	fprintf(f, "DBF(%s, t = %lu) = %lu\n", task->t_name, t, demand);
	fprintf(f, "        | %5lu - %-5lu     |\n", t, task->t_deadline);
	fprintf(f, "%5lu * | ------------- + 1 | = %lu * (%lu + 1) = %5lu\n", wcet,
		wcet, frac, demand);
	fprintf(f, "        |  %8lu         |\n", task->t_period);
	fprintf(f, "        +-                 -+\n");
	

	return demand;
}

int
task_merge(task_t* task) {
	tint_t m = task->t_threads;
	tint_t wcet = task->wcet(m);

	task_threads(task, 1);
	task->wcet(1) = wcet;
}

int
task_is_constrained(task_t* task) {
	if (task->t_deadline <= task->t_period) {
		return 1;
	}
	return 0;
}
