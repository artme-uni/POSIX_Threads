#include "dynamic_distribution.h"

pthread_mutex_t mutex;
int pr_task_iterator;
int pr_unused_task_count;

int *pr_task_weight;
double pr_task_result = 0;
int pr_done_task_count = 0;

int create_thread(pthread_t *thread, void *(*func)(void *))
{
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);

    pthread_mutex_init(&mutex, NULL);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_JOINABLE);

    pthread_create(thread, &attrs, func, NULL);

    pthread_attr_destroy(&attrs);

    return 0;
}

int print_proc_report(int pr_rank, int comm_size, double time_iter_taken)
{
    for (int k = 0; k < comm_size; ++k)
    {
        if (k == pr_rank)
        {
            printf("\n");
            printf("- Process RANK : %d\n", pr_rank);
            printf("- Task done : %d\n", pr_done_task_count);
            printf("- Time taken : %lf\n", time_iter_taken);
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }
    return 0;
}

int print_list_report(int pr_rank, int comm_size, int current_list, double time_iter_end, double time_iter_start)
{
    double time_iter_taken = time_iter_end - time_iter_start;

    double time_iter_min = 0;
    double time_iter_max = 0;
    MPI_Allreduce(&time_iter_taken, &time_iter_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
    MPI_Allreduce(&time_iter_taken, &time_iter_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    double imbalance_time = time_iter_max - time_iter_min;

    double global_task_result = 0;
    MPI_Allreduce(&pr_task_result, &global_task_result, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    if (pr_rank == 0)
    {
        printf("\n");
        printf("\t\tLIST %d HAS DONE\n", current_list + 1);
        printf("\n");
        printf("* Global result : %lf\n", global_task_result);
    }

    print_proc_report(pr_rank, comm_size, time_iter_taken);

    if (pr_rank == 0)
    {
        printf("\n");
        printf("* Imbalance time: %lf\n", imbalance_time);
        printf("* Imbalance proportion: %d %%\n", (int) ((imbalance_time / time_iter_max) * 100));
        printf("\n");
    }

    return 0;
}

int init_repeat_count(int *repeat_count, int pr_task_count, int pr_rank, int current_list)
{
    int x = pr_task_count * pr_rank;

    for (int i = 0; i < pr_task_count; ++i)
    {
        int a = TASK_COUNT * (current_list+1) / LIST_COUNT;
        repeat_count[i] = TASK_COUNT_RATE * exp(-3*pow(x - a, 2) / pow(TASK_COUNT, 2));
        x++;
    }

/*for (int i = 0; i < pr_task_count; ++i)
    {
        repeat_count[i] = abs(50 - i % 100) * abs(pr_rank - (current_list % comm_size)) * TASK_COUNT_RATE;
    }*/

    return 0;
}



int execute_lists(int pr_rank, int comm_size, int pr_task_count)
{
    int current_list = 0;
    while (current_list != LIST_COUNT)
    {
        double time_iter_start = MPI_Wtime();
        pr_done_task_count = 0;
        pr_task_result = 0;

        init_repeat_count(pr_task_weight, pr_task_count, pr_rank, current_list);

        int average_weight = 0;
        for (int i = 0; i < pr_task_count; ++i)
        {
            average_weight += pr_task_weight[i];
        }
        average_weight /= pr_task_count;
        printf("list %d - pr %d - weight %d\n", current_list, pr_rank, average_weight);


        pthread_mutex_lock(&mutex);
        pr_unused_task_count = pr_task_count;
        pthread_mutex_unlock(&mutex);
        run_tasks(pr_task_weight);

        get_additional_tasks(pr_rank, comm_size);

        double time_iter_end = MPI_Wtime();

        print_list_report(pr_rank, comm_size, current_list, time_iter_end, time_iter_start);

        MPI_Barrier(MPI_COMM_WORLD);
        current_list++;
    }

    return 0;
}

int get_pr_task_count(int pr_rank, int comm_size)
{
    int pr_task_count = TASK_COUNT / comm_size;

    if (pr_rank < TASK_COUNT % comm_size)
        pr_task_count++;

    return pr_task_count;
}

int run_tasks(const int *repeat_count)
{
    pr_task_iterator = 0;

    while (pr_unused_task_count)
    {
        pthread_mutex_lock(&mutex);
        int current_repeat_count = repeat_count[pr_task_iterator++];
        pr_unused_task_count--;
        pthread_mutex_unlock(&mutex);

        for (int i = 0; i < current_repeat_count; ++i)
        {
            pr_task_result += sin(i);
        }

        pr_done_task_count++;
    }

    return 0;
}

int get_additional_tasks(int pr_rank, int comm_size)
{
    bool is_needed_to_work = true;
    while (is_needed_to_work)
    {
        is_needed_to_work = false;

        for (int i = (pr_rank + 1) % comm_size; i != pr_rank; i = (i + 1) % comm_size)
        {
            MPI_Send(&pr_rank, 1, MPI_INT, i, TAG_PROC_RANK, MPI_COMM_WORLD);

            int additional_task_count = 0;
            MPI_Recv(&additional_task_count, 1, MPI_INT, i, TAG_TASK_COUNT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (additional_task_count != 0)
            {
                MPI_Recv(pr_task_weight, additional_task_count, MPI_INT, i, TAG_TASK_WEIGHT+additional_task_count, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);

                pthread_mutex_lock(&mutex);
                pr_unused_task_count += additional_task_count;
                pthread_mutex_unlock(&mutex);

                run_tasks(pr_task_weight);
                is_needed_to_work = true;
            }
        }
    }
}
