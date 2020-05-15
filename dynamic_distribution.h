#ifndef POSIX_THREADS_DYNAMIC_DISTRIBUTION_H
#define POSIX_THREADS_DYNAMIC_DISTRIBUTION_H

#include<cstdio>
#include <mpi.h>
#include <pthread.h>
#include <cstdlib>
#include <cmath>

#define TASK_COUNT 1000
#define LIST_COUNT 3
#define TASK_COUNT_RATE 1000

#define TAG_PROC_RANK 1
#define TAG_TASK_COUNT 2
#define TAG_TASK_WEIGHT 3

#define CMD_EXIT_THREAD -7

extern pthread_mutex_t mutex;
extern int pr_task_iterator;
extern int pr_unused_task_count;

extern int *pr_task_weight;
extern double pr_task_result;
extern int pr_done_task_count;

int create_thread(pthread_t *thread, void* (*func)(void*));

int print_proc_report(int pr_rank, int comm_size, double time_iter_taken);

int print_list_report(int pr_rank, int comm_size, int current_list, double time_iter_end, double time_iter_start);

int init_repeat_count(int *repeat_count, int pr_task_count, int pr_rank, int comm_size, int current_list);

int run_tasks(const int *repeat_count);

int execute_lists(int pr_rank, int comm_size, int pr_task_count);

int get_pr_task_count(int pr_rank, int comm_size);

int get_additional_tasks(int pr_rank, int comm_size);

#endif //POSIX_THREADS_DYNAMIC_DISTRIBUTION_H
