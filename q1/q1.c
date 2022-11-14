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
#define MAX 100

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

int time_wasted = 0;
int disappointed = 0;

pthread_cond_t condition_machine;

typedef struct student
{
    int arr_time; // time of arrival
    pthread_t t;
    int id; // for identifying the student
    int thread_id;
    int alloc_time;           // time when machine gets allocated
    int wash_time;            // time taken to wash clothes
    int patience;             // patience
    pthread_mutex_t st_mutex; // mutex lock for student
} student;

int N, M; // N: students, M: washing machines
int machines_avail;

long int globalstart;
student *student_ptr[MAX];

pthread_mutex_t time_mutex; // to manage incrementing/decrementing time
// pthread_mutex_t goal_time_mutex;
pthread_mutex_t machine_mutex; // to manage availability of machines
pthread_mutex_t arrival_mutex;
pthread_mutex_t wash_mutex;
pthread_mutex_t wash_mutex2;

pthread_cond_t for_stay;

sem_t machines;

// int comparator(const void *a, const void *b)
// {
//     struct student* x = (struct student *)a;
//     struct student* y = (struct student *)b;
//     printf("%d\n", ((struct student *)a)->arr_time);
//     if (x->arr_time == y->arr_time)
//     {
//         return x->id - y->id;
//     }
//     return (x->arr_time - y->arr_time);
// }

void *student_init(void *args)
{
    int id = *((int *)args);

    // sleep(student_ptr[id]->arr_time);
    struct timespec delay = {student_ptr[id]->arr_time, 1000000 * (id+2)};
    pselect(0, NULL, NULL, NULL, &delay, NULL);

    pthread_mutex_lock(&arrival_mutex);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    pthread_mutex_unlock(&arrival_mutex);
    white();
    // printf("%ld: Student %d arrives\n", ts1.tv_sec - globalstart, id + 1);
    printf("%d: Student %d arrives\n", student_ptr[id]->arr_time, student_ptr[id]->id + 1);
    reset();

    pthread_mutex_lock(&machine_mutex);
    // printf("Patience time %ld of id %d\n", important_ts.tv_sec - globalstart, id + 1);

    int rc = -1;
    while (machines_avail == 0)
    {
        struct timespec ts1;
        clock_gettime(CLOCK_REALTIME, &ts1);
        ts1.tv_sec = globalstart + student_ptr[id]->arr_time + student_ptr[id]->patience;
        rc = pthread_cond_timedwait(&condition_machine, &machine_mutex, &ts1);
        if (rc == ETIMEDOUT)
        {
            goto end;
        }
    }

    // clock_gettime(CLOCK_REALTIME, &ts);

    // pthread_mutex_unlock(&machine_mutex);
    // pthread_mutex_lock(&machine_mutex);
    if (sem_trywait(&machines) == false)
    {
        machines_avail--;
        pthread_mutex_unlock(&machine_mutex);

        struct timespec ts1;

        pthread_mutex_lock(&wash_mutex);
        clock_gettime(CLOCK_REALTIME, &ts1);
        pthread_mutex_unlock(&wash_mutex);
        green();
        printf("%ld: Student %d starts washing\n", ts1.tv_sec - globalstart, student_ptr[id]->id + 1);
        reset();

        ts1.tv_sec -= globalstart;
        pthread_mutex_lock(&wash_mutex2);
        time_wasted += ts1.tv_sec - student_ptr[id]->arr_time;
        pthread_mutex_unlock(&wash_mutex2);

        sleep(student_ptr[id]->wash_time);

        pthread_mutex_lock(&machine_mutex);
        machines_avail++;
        pthread_mutex_unlock(&machine_mutex);

        sem_post(&machines);
        pthread_cond_signal(&condition_machine);

        struct timespec ts2;

        // pthread_mutex_lock(&wash_mutex2);
        clock_gettime(CLOCK_REALTIME, &ts2);
        // pthread_mutex_unlock(&wash_mutex2);
        yellow();
        printf("%ld: Student %d leaves after washing\n", ts2.tv_sec - globalstart, student_ptr[id]->id + 1);
        reset();
        return NULL;
    }
    pthread_mutex_unlock(&machine_mutex);

    // did_not_wash:;
end:;
    clock_gettime(CLOCK_REALTIME, &ts);
    pthread_mutex_lock(&wash_mutex2);
    ts.tv_sec -= globalstart;
    time_wasted += student_ptr[id]->patience;
    disappointed += 1;
    pthread_mutex_unlock(&wash_mutex2);

    red();
    printf("%ld: Student %d leaves without washing\n", ts.tv_sec, student_ptr[id]->id + 1);
    reset();
    return NULL;
}

void sem_initialisation()
{
    sem_init(&machines, 0, M);
    pthread_mutex_init(&time_mutex, NULL);
    pthread_mutex_init(&machine_mutex, NULL);
    pthread_mutex_init(&arrival_mutex, NULL);
    pthread_mutex_init(&wash_mutex, NULL);
    pthread_mutex_init(&wash_mutex2, NULL);

    pthread_cond_init(&for_stay, NULL);
    pthread_cond_init(&condition_machine, NULL);

    machines_avail = M;
}

int main()
{
    srand(time(NULL));
    struct timespec ts;
    int j;

    scanf("%d %d", &N, &M);
    j = 0;
    while (j < N)
    {
        student_ptr[j] = (student *)malloc(sizeof(student));
        j++;
        scanf("%d %d %d", &student_ptr[j - 1]->arr_time, &student_ptr[j - 1]->wash_time, &student_ptr[j - 1]->patience);
        student_ptr[j - 1]->id = j - 1;
    }
    fflush(stdout);

    sem_initialisation();
    for (int a = 0; a < N-1; a++)
    {
        for (int b = 0; b < N-1; b++)
        {
            if ((student_ptr[b]->arr_time > student_ptr[b+1]->arr_time) || ((student_ptr[a]->arr_time == student_ptr[b]->arr_time && ((student_ptr[a]->id < student_ptr[b]->id)))))
            {
                struct student *temp = student_ptr[b];
                student_ptr[b] = student_ptr[a];
                student_ptr[a] = temp;
            }
        }
    }
    // qsort(student_ptr, N, sizeof(student *), comparator);
    // for (int i = 0; i < N; i++)
    // {
    //     printf("%d\n", student_ptr[i]->id);
    // }

    clock_gettime(CLOCK_REALTIME, &ts);
    globalstart = ts.tv_sec;

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
    reset();
    printf("%d\n", disappointed);
    printf("%d\n", time_wasted);
    double x = (double)disappointed / (double)N;
    x *= 100;
    if (x < 25)
    {
        printf("No\n");
    }
    else
    {
        printf("Yes\n");
    }

    return 0;
}