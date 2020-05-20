
#include<cstdio>
#include "dynamic_distribution.h"

void *send_thread_func(void *arg)
{
    while (true)
    {
        int destination_pr_rank;

        MPI_Recv(&destination_pr_rank, 1, MPI_INT, MPI_ANY_SOURCE, TAG_PROC_RANK, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
        if (destination_pr_rank == CMD_EXIT_THREAD)
        {
            break;
        }

        int send_task_count = pr_unused_task_count / 2;

        MPI_Send(&send_task_count, 1, MPI_INT, destination_pr_rank, TAG_TASK_COUNT, MPI_COMM_WORLD);
        MPI_Send(&pr_task_weight[pr_task_iterator + 1], send_task_count, MPI_INT, destination_pr_rank,
                 TAG_TASK_WEIGHT + send_task_count, MPI_COMM_WORLD);

        pthread_mutex_lock(&mutex);
        pr_unused_task_count -= send_task_count;
        pr_task_iterator += send_task_count;
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void *worker_thread_func(void *arg)
{
    int comm_size, pr_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &pr_rank);

    int pr_task_count = get_pr_task_count(pr_rank, comm_size);
    pr_unused_task_count = pr_task_count;

    pr_task_weight = (int *) calloc(pr_task_count, sizeof(int));

    double time_global_start = MPI_Wtime();
    execute_lists(pr_rank, comm_size, pr_task_count);


    int cmd = CMD_EXIT_THREAD;
    MPI_Send(&cmd, 1, MPI_INT, pr_rank, TAG_PROC_RANK, MPI_COMM_WORLD);

    double time_global_end = MPI_Wtime();
    if (pr_rank == 0)
    {
        printf("+ Lists execution time : %lf\n", time_global_end - time_global_start);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, NULL);

    pthread_t send_thread;
    create_thread(&send_thread, send_thread_func);

    pthread_t worker_thread;
    create_thread(&worker_thread, worker_thread_func);

    pthread_join(worker_thread, NULL);
    pthread_join(send_thread, NULL);

    free(pr_task_weight);

    pthread_mutex_destroy(&mutex);

    MPI_Finalize();
    return 0;
}