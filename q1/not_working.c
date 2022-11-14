#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define true 1
#define false 0

// colours for printing
void yellow()
{
    printf("\x1b[1;33m");
}
void red()
{
    printf("\x1b[1;31m");
}
void green()
{
    printf("\x1b[1;32m");
}
void white()
{
    printf("\033[0;37m");
}
void reset()
{
    printf("\x1b[0m");
}
pthread_cond_t condition_machine = PTHREAD_COND_INITIALIZER;
#define MAX 100

typedef struct student
{
    pthread_t t;
    int id; // for identifying the student
    int thread_id;
    int arr_time;             // time of arrival
    int alloc_time;           // time when machine gets allocated
    int wash_time;            // time taken to wash clothes
    int patience;             // patience
    pthread_mutex_t st_mutex; // mutex lock for student
} student;

int N, M; // N: students, M: washing machines
int machines_avail;
// int curr_time;
int goal_curr_time;
int count;
long int globalstart;
student *student_ptr[MAX];

pthread_mutex_t time_mutex; // to manage incrementing/decrementing time
pthread_mutex_t goal_time_mutex;
pthread_mutex_t machine_mutex; // to manage availability of machines
pthread_mutex_t another_one;
pthread_cond_t for_stay;

sem_t machines;

void *student_init(void *args)
{
    int id = *((int *)args);

    // pthread_mutex_lock(&time_mutex);
    // int current_time = curr_time;
    // pthread_mutex_unlock(&time_mutex);
    // // printf("arrival time for student %d: %d\n", id+1, student_ptr[id]->arr_time);

    // if (student_ptr[id]->arr_time > current_time){
    //     int arr_time = student_ptr[id]->arr_time;
    //     sleep(student_ptr[id]->arr_time - current_time);
    // }
    // sleep(student_ptr[id]->arr_time);
    struct timespec delay = {student_ptr[id]->arr_time, 1000000 * (id + 2)};
    pselect(0, NULL, NULL, NULL, &delay, NULL);
    // pthread_mutex_lock(&time_mutex);
    // current_time = curr_time;
    // pthread_mutex_unlock(&time_mutex);
    struct timespec ts1;
    clock_gettime(CLOCK_REALTIME, &ts1);
    white();
    printf("%ld: Student %d arrives\n", ts1.tv_sec - globalstart, id + 1);
    reset();

    struct timespec ts;
    // int x = clock_gettime(CLOCK_REALTIME, &ts);
    // if (x == -1)
    // {
    //     return NULL;
    // }
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        /* handle error */
        return NULL;
    }
    ts.tv_sec += student_ptr[id]->patience;

    int s = 0;
    // while ((s = sem_timedwait(&machines, &ts)) == -1 && (errno == EINTR))
    // {
    //     continue; // restart if interrupted by handler
    // }

    // pthread_mutex_lock(&time_mutex);
    // current_time = curr_time;
    // pthread_mutex_unlock(&time_mutex);
    clock_gettime(CLOCK_REALTIME, &ts1);

    // if (s == -1)
    // {
    //     if (errno != ETIMEDOUT)
    //         perror("sem_timedwait");
    //     else
    //     {
    //         // patience of student has run out
    //         red();
    //         printf("%d: Student %d leaves without washing\n", ts1.tv_sec - globalstart, id + 1);
    //         reset();
    //     }
    // }
    // if (machines_avail == 0)
    // {
    // while ((s = sem_timedwait(&machines, &ts)) == -1 && (errno == EINTR))
    // {
    //     continue; // restart if interrupted by handler
    // }
    // printf("%d Machine leaves\n", id + 1);
    long int time_saver = ts1.tv_sec;
    struct timespec important_ts;
    clock_gettime(CLOCK_REALTIME, &important_ts);

    important_ts.tv_sec += student_ptr[id]->patience;
    pthread_mutex_lock(&machine_mutex);
    // printf("Patience time %ld of id %d\n", important_ts.tv_sec - globalstart, id + 1);

    int rc = -1;
    while (machines_avail == 0)
    {
        rc = pthread_cond_timedwait(&condition_machine, &machine_mutex, &important_ts);
        if (rc == ETIMEDOUT)
        {
            break;
        }
    }

    clock_gettime(CLOCK_REALTIME, &ts1);
    // if (ts1.tv_sec > time_saver + student_ptr[id]->patience )
    // // if(rc==ETIMEDOUT && machines_avail==0)
    // {
    //         pthread_mutex_unlock(&machine_mutex);

    //     // printf("patience time is %ld\n", important_ts.tv_sec - globalstart);
    //     printf("time is %ld\n", ts1.tv_sec - globalstart);
    //     // }
    //     // if (ts1.tv_sec > time_saver + student_ptr[id]->patience)
    //     // {
    //     // printf("time: %ld\n", ts1.tv_sec - globalstart);
    //     // printf("patience\n");
    //     // pthread_mutex_unlock(&machine_mutex);

    //     clock_gettime(CLOCK_REALTIME, &ts1);

    //     // patience of student has run out
    //     red();
    //     printf("%ld: Student %d leaves without washing\n", ts1.tv_sec - globalstart, id + 1);
    //     reset();

    //     pthread_mutex_lock(&another_one);
    //     count++;
    //     pthread_mutex_unlock(&another_one);
    //     return NULL;
    // }
    pthread_mutex_unlock(&machine_mutex);

    // if (s == -1)
    // {
    //     // if (errno != ETIMEDOUT)
    //     //     perror("sem_timedwait");
    //     // else
    //     // {
    //         clock_gettime(CLOCK_REALTIME, &ts1);

    //         // patience of student has run out
    //         red();
    //         printf("%d: Student %d leaves without washing\n", ts1.tv_sec - globalstart, id + 1);
    //         reset();

    //         pthread_mutex_lock(&another_one);
    //         count++;
    //         pthread_mutex_unlock(&another_one);
    //         return NULL;
    //     // }
    // }

    // else
    // {
    int check = 0;
    pthread_mutex_lock(&machine_mutex);
    if (machines_avail > 0)
    {
        clock_gettime(CLOCK_REALTIME, &ts1);

        machines_avail--;
        check = 1;
        green();
        printf("%ld: Student %d starts washing\n", ts1.tv_sec - globalstart, id + 1);
        reset();
        // sem_wait(&machines);
        pthread_mutex_unlock(&machine_mutex);
        sleep(student_ptr[id]->wash_time);
        pthread_mutex_lock(&machine_mutex);
        machines_avail++;
        // sem_post(&machines);

        pthread_mutex_unlock(&machine_mutex);

        clock_gettime(CLOCK_REALTIME, &ts1);
        yellow();
        printf("%ld: Student %d leaves after washing\n", ts1.tv_sec - globalstart, id + 1);
        reset();
        pthread_cond_signal(&condition_machine);
    }
    else
    {
        pthread_mutex_unlock(&machine_mutex);
        // printf("time is %ld\n", ts1.tv_sec - globalstart);
        // }
        // if (ts1.tv_sec > time_saver + student_ptr[id]->patience)
        // {
        // printf("time: %ld\n", ts1.tv_sec - globalstart);
        // printf("patience\n");
        // pthread_mutex_unlock(&machine_mutex);

        clock_gettime(CLOCK_REALTIME, &ts1);

        // patience of student has run out
        red();
        printf("%ld: Student %d leaves without washing\n", ts1.tv_sec - globalstart, id + 1);
        reset();

        pthread_mutex_lock(&another_one);
        count++;
        pthread_mutex_unlock(&another_one);
        return NULL;
    }
    // if (check == 1)

    // int c_time;
    // pthread_mutex_lock(&goal_time_mutex);
    // c_time = goal_curr_time;
    // c_time += student_ptr[id]->wash_time;
    // pthread_mutex_unlock(&goal_time_mutex);

    // pthread_mutex_lock(&goal_time_mutex);
    // while (c_time > goal_curr_time)
    // {
    //     pthread_cond_wait(&for_stay, &goal_time_mutex);
    // }
    // pthread_mutex_unlock(&goal_time_mutex);

    // yellow();
    // printf(" Student %d leaves after washing\n", id + 1);
    // reset();

    // pthread_mutex_lock(&machine_mutex);
    // if (check == 1)
    // {
    //     machines_avail++;
    //     sem_post(&machines);
    // }
    // pthread_mutex_unlock(&machine_mutex);
    // }

    pthread_mutex_lock(&another_one);
    count++;
    pthread_mutex_unlock(&another_one);
    return NULL;
}

void sem_initialisation()
{
    sem_init(&machines, 0, M);
    pthread_mutex_init(&time_mutex, NULL);
    pthread_mutex_init(&goal_time_mutex, NULL);
    pthread_mutex_init(&machine_mutex, NULL);
    pthread_mutex_init(&another_one, NULL);
    pthread_cond_init(&for_stay, NULL);
    machines_avail = M;
}

int main()
{
    struct timespec sts;
    clock_gettime(CLOCK_REALTIME, &sts);
    globalstart = sts.tv_sec;
    srand(time(NULL));
    // curr_time = 0;
    goal_curr_time = 0;
    int j;

    scanf("%d %d", &N, &M);
    j = 0;
    while (j < N)
    {
        student_ptr[j] = (student *)malloc(sizeof(student));
        // student_ptr[i]->alloc_time = -1;
        j++;
        scanf("%d %d %d", &student_ptr[j - 1]->arr_time, &student_ptr[j - 1]->wash_time, &student_ptr[j - 1]->patience);
        student_ptr[j - 1]->id = j - 1;
    }
    fflush(stdout);

    printf("Simulation beginning.\n");
    sem_initialisation();
    for (int a = 0; a < N - 1; a++)
    {
        for (int b = 0; b < N - 1; b++)
        {
            if ((student_ptr[b]->arr_time > student_ptr[b + 1]->arr_time) || ((student_ptr[a]->arr_time == student_ptr[b]->arr_time && ((student_ptr[a]->id < student_ptr[b]->id)))))
            {
                struct student *temp = student_ptr[b];
                student_ptr[b] = student_ptr[a];
                student_ptr[a] = temp;
            }
        }
    }

    count = 0;
    j = 0;
    while (j < N)
    {
        j++;
        pthread_mutex_init(&(student_ptr[j - 1]->st_mutex), NULL);
        student_ptr[j - 1]->thread_id = pthread_create(&(student_ptr[j - 1]->t), NULL, student_init, (void *)(&(student_ptr[j - 1]->id)));
        sleep(0.001);
        // struct timespec delay = {0, 200000};
        // pselect(0, NULL, NULL, NULL, &delay, NULL);
    }

    j = 0;
    while (j < N)
    {
        j++;
        pthread_join(student_ptr[j - 1]->t, NULL);
    }

    return 0;
}
/*
machine()
{
    p_cond_waits()
    sleep(washing_time)
machine_available++;
}
student()
{
    while(machine_availble==0)
    {
        p_cond_wait()
    }
}
*/