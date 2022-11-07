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

#define false 0
#define true 1

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
void blue()
{
    printf("\x1b[1;34m");
}
/*
M pizzas -> prep time for each, if the pizza requires limited
    ingredients, it uses a single unit of the limited ingredient
N chefs -> arr time, exit time, time to bake pizza, if incomig chefs,
    customers wait, else cutomers rejected; a chef is only assigned an order
    if they have enough time
K cars in drive-thru -> fcfs:
    customers can order multiple pizzas of diff kinds, if rejected,
    exit the drive thru by overtaking; else s seconds to reach pickup point
    if pizza not present, wait
Ingredients -> some are unlimited, some limited for the day, if limited
    ingredients are over restaurant shuts down;
    only drive thru orders having at least one pizza which can be made are
    accepted, other rejected
Ovens -> L ovens; can cook 1 pizza at a time;
    for prep time t: 3 seconds to prepare ingredients -> alloting of oven ->
    (t-3) seconds to bake
Pick up spot -> no restrictions
*/

typedef struct pizza
{
    int id;
    int prep_time;
    int *ingredients;
    int num_ingredients;
    int assigned_chef;
} pizza;

typedef struct chef
{
    int id;
    int arr_time;
    int exit_time;
    int status; // 0-> sleeping, 1->cooking
    int *rejectArrPizzId;
    int rejectIndex;
    pthread_t chef_thread;
    int thread_id;
    pthread_mutex_t chef_mutex;
} chef;

typedef struct ingredient
{
    int id;
    int qty;
} ingredient;

typedef struct assigned_pizza
{
    int id;
    int assigned_chef;
    int order_id;
    int status; // 1 -> initialised, 2 -> assigned to chef, 3 -> completed, 4 -> rejected
} assigned_pizza;

typedef struct order
{
    int id;
    int customer_id;
    int arr_time;
    int status; // 1-> initialised, 2->queue, 3->accepted, 4->rejected, 5->partially completed, 6 -> cooked
    int num_pizzas;
    int is_accpeted; // 1-> accepted, -1->rejected
    int num_pizzas_processed;
    int num_pizzas_made;
    int new_pizza;
    assigned_pizza *pizzas;
    pthread_t order_thread;
    int thread_id;
    pthread_mutex_t order_mutex;
    pthread_cond_t order_cond;
} order;

int globaltime;

int num_pizza_varieties;      // M
int num_chefs;                // N
int num_limited_ingreds;      // i
int num_customers;            // c
int num_ovens;                // o
int reach_pickup_time;        // k
int drive_thru_max_customers; // not given in input --> check
int drive_thru_customers;

int chefs_working;
int activeChefs = 0;

order *order_ptr;
chef *chef_ptr;
pizza *pizza_ptr;
ingredient *ingredient_ptr;
int *oven_ptr;

pthread_mutex_t *write_pizza_locks;
pthread_mutex_t pizza_queue;
pthread_mutex_t drive_thru;
pthread_mutex_t chefs_check;
pthread_mutex_t restaurant_check;
pthread_mutex_t ingredient_lock;
pthread_mutex_t oven_mutex;

pthread_cond_t drive_next;
pthread_cond_t chef_cond;
pthread_cond_t chef_cond_for_oven;

int *order_pizza_q;
int *order_pizza_id_q;
int REAR = -1;
int FRONT = -1;
int SIZE = 0;

void enqueue(int x, int y)
{
    if (REAR != SIZE - 1)
    {
        order_pizza_q[REAR + 1] = x;
        order_pizza_id_q[REAR + 1] = y;
        REAR++;
        if (FRONT < 0)
            FRONT = 0;
    }
}

void dequeue()
{
    if (FRONT != -1 && FRONT <= REAR)
    {
        if ((FRONT + 1) > REAR)
            FRONT = REAR - 1;
        else
            FRONT++;
    }
}

void display()
{
    if (REAR != -1)
    {
        printf("\nQueue elements are:\n");
        int i = FRONT;
        while (i <= REAR)
        {
            printf("%d %d \n", order_pizza_q[i], order_pizza_id_q[i]);
            i++;
        }
    }
    else
    {
        printf("\nQueue is empty!\n");
    }
}

int MAX(int x, int y)
{
    if (x >= y)
    {
        return x;
    }
    else
    {
        return y;
    }
}

void get_input()
{
    drive_thru_max_customers = 5;
    scanf("%d %d %d %d %d %d", &num_chefs, &num_pizza_varieties, &num_limited_ingreds, &num_customers, &num_ovens, &reach_pickup_time);

    order_ptr = (order *)malloc(sizeof(order) * num_customers);
    chef_ptr = (chef *)malloc(sizeof(chef) * num_chefs);
    pizza_ptr = (pizza *)malloc(sizeof(pizza) * num_pizza_varieties);
    ingredient_ptr = (ingredient *)malloc(sizeof(ingredient) * num_limited_ingreds);
    oven_ptr = (int *)malloc(sizeof(int) * num_ovens);
    chefs_working = num_chefs;
    write_pizza_locks = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * num_customers);
    // order_cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t)*num_customers);

    drive_thru_customers = 0;

    int i = 0;
    while (i < num_ovens)
    {
        oven_ptr[i] = false;
        i++;
    }

    i = 0;
    while (i < num_pizza_varieties)
    {
        scanf("%d %d %d", &pizza_ptr[i].id, &pizza_ptr[i].prep_time, &pizza_ptr[i].num_ingredients);
        int x = pizza_ptr[i].num_ingredients;
        pizza_ptr[i].ingredients = (int *)malloc(sizeof(int) * x);
        int j = 0;
        while (j < x)
        {
            scanf("%d", &pizza_ptr[i].ingredients[j]);
            j++;
        }
        pizza_ptr[i].assigned_chef = -1;
        i++;
    }
    i = 0;
    while (i < num_limited_ingreds)
    {
        scanf("%d", &ingredient_ptr[i].qty);
        ingredient_ptr[i].id = i + 1;
        i++;
    }
    i = 0;
    while (i < num_chefs)
    {
        scanf("%d %d", &chef_ptr[i].arr_time, &chef_ptr[i].exit_time);
        chef_ptr[i].id = i + 1;
        chef_ptr[i].status = 0; // sleeping
        int x = chef_ptr[i].arr_time;
        if (x == 0)
            activeChefs++;
        chef_ptr[i].rejectIndex = 0;
        chef_ptr[i].rejectArrPizzId = (int *)malloc(4 * num_pizza_varieties * num_customers);
        i++;
    }

    i = 0;
    while (i < num_customers)
    {
        scanf("%d %d", &order_ptr[i].arr_time, &order_ptr[i].num_pizzas);
        int x = order_ptr[i].num_pizzas;
        order_ptr[i].pizzas = (assigned_pizza *)malloc(sizeof(assigned_pizza) * x);

        int j = 0;
        while (j < x)
        {
            scanf("%d", &order_ptr[i].pizzas[j].id);
            order_ptr[i].pizzas[j].status = 1; // initialised
            j++;
        }
        order_ptr[i].id = i + 1;
        order_ptr[i].status = 1; // initialised
        order_ptr[i].num_pizzas_made = 0;
        order_ptr[i].num_pizzas_processed = 0;
        order_ptr[i].is_accpeted = true;
        order_ptr[i].new_pizza = 0;
        pthread_cond_init(&(order_ptr[i].order_cond), NULL);
        i++;
    }
    i = 0;
    int total = 0;
    while (i < num_customers)
    {
        total += order_ptr[i].num_pizzas;
        i++;
    }
    SIZE = total;
    order_pizza_q = (int *)malloc(4 * total);
    order_pizza_id_q = (int *)malloc(4 * total);
}

int check_order_status(int id)
{
    int x = order_ptr[id].num_pizzas_processed;
    int y = order_ptr[id].num_pizzas;
    int z = order_ptr[id].num_pizzas_made;

    if (x != y)
    {
        return 0;
    }
    else
    {
        if (z == x)
        {
            order_ptr[id].status = 6;
        }
        else if (z < x)
        {
            order_ptr[id].status = 5;
        }
        else
        {
            order_ptr[id].status = 4;
        }
        return 1;
    }
}

int check_pizza_reject(int id, int id2)
{
    int i = 0;
    int x = chef_ptr[id].rejectIndex;
    while (i < x)
    {
        int y = chef_ptr[id].rejectArrPizzId[i];
        if (y == id2)
        {
            return 1;
        }
        i++;
    }
    return 0;
}

void *chefinit(void *args)
{
    int id = *((int *)args);

    int rejected_due_to_time = false;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int curr_time = ts.tv_sec - globaltime;
    sleep(chef_ptr[id].arr_time);

    clock_gettime(CLOCK_REALTIME, &ts);
    curr_time = ts.tv_sec;
    blue();
    printf("Chef %d arrives at time %d\n", id, chef_ptr[id].arr_time);
    reset();

    pthread_mutex_lock(&restaurant_check);
    activeChefs++;
    pthread_mutex_lock(&restaurant_check);

start:
    pthread_mutex_lock(&(chef_ptr[id].chef_mutex));
    pthread_mutex_lock(&pizza_queue);

    struct timespec ts1;
    clock_gettime(CLOCK_REALTIME, &ts1);
    int wait_time = ts1.tv_sec;
    wait_time -= (chef_ptr[id].arr_time + globaltime);

    clock_gettime(CLOCK_REALTIME, &ts1);
    int wait_time2 = chef_ptr[id].exit_time - chef_ptr[id].arr_time - wait_time;
    ts1.tv_sec += wait_time2;

    int rc;
    ///**********************
    /// BUSY WAITING?
    ////*******************
    while (check_pizza_reject(id, order_pizza_q[FRONT]) == 1 || FRONT == -1)
    {
        clock_gettime(CLOCK_REALTIME, &ts);
        int curr_time = ts.tv_sec - globaltime;
        if (curr_time >= chef_ptr[id].exit_time)
            break;
        rc = pthread_cond_timedwait(&chef_cond, &pizza_queue, &ts1);
    }

    clock_gettime(CLOCK_REALTIME, &ts);
    curr_time = ts.tv_sec - globaltime;
    if (curr_time >= chef_ptr[id].exit_time)
    {
        pthread_mutex_lock(&restaurant_check);
        activeChefs -= 1;
        chefs_working -= 1;
        pthread_mutex_unlock(&restaurant_check);

        if (chefs_working == 0)
        {
            int i = 0;
            while (i < num_customers)
            {
                pthread_cond_signal(&order_ptr[i].order_cond);
                i++;
            }
        }

        clock_gettime(CLOCK_REALTIME, &ts);
        curr_time = ts.tv_sec - globaltime;
        blue();
        printf("Chef %d exits at time %d\n", id, curr_time);
        reset();

        pthread_mutex_unlock(&restaurant_check);
        pthread_mutex_unlock(&pizza_queue);

        return NULL;
    }

    int pizza_id = order_pizza_q[FRONT];
    int order_id = order_pizza_id_q[FRONT];
    int total_pizzas = order_ptr[order_id].num_pizzas;

    dequeue;
    pthread_mutex_unlock(&pizza_queue);

    clock_gettime(CLOCK_REALTIME, &ts);
    int check_time = (chef_ptr[id].arr_time + pizza_ptr[pizza_id].prep_time) - chef_ptr[id].exit_time; // time left comapred with time taken to prepare pizza
    if (check_time > 0)
    {
        blue();
        printf("Chef %d is rejecting the pizza %d from order %d due to lack of time.\n", id, pizza_id, order_id);
        reset();
        int idx = chef_ptr[id].rejectIndex;
        chef_ptr[id].rejectArrPizzId[idx] = pizza_id;
        chef_ptr[id].rejectIndex++;
        rejected_due_to_time = true;
        goto end;
    }

    blue();
    printf("Chef %d is preparing the pizza %d from order %d.\n", id, pizza_id, order_id);
    reset();

    pthread_mutex_lock(&ingredient_lock);
    int i = 0;
    while (i < order_ptr[order_id].num_pizzas)
    {
        int pid = order_ptr[order_id].pizzas[i].id;
        if (pizza_id == pid)
        {
            int checker = true;
            int k = 0;
            while (k < pizza_ptr[pizza_id].num_ingredients)
            {
                if (ingredient_ptr[pizza_ptr[pizza_id].ingredients[k] - 1].qty == 0)
                {
                    checker = false;
                    break;
                }
                k++;
            }
            if (checker == false)
            {
                blue();
                printf("Chef %d is rejecting the pizza %d from order %d due to lack of ingredients.\n", id + 1, pizza_id, order_id);
                reset();
                order_ptr[order_id].pizzas[i].status = 4; // rejected
                pthread_mutex_unlock(&ingredient_lock);
                goto end;
            }
            k = 0;
            while (k < pizza_ptr[pizza_id].num_ingredients)
            {
                ingredient_ptr[pizza_ptr[pizza_id].ingredients[k]].qty--;
                k++;
            }
            break;
        }
        i++;
    }
    pthread_mutex_unlock(&ingredient_lock);
    sleep(3); // time taken to get hold of ingredients

    pthread_mutex_lock(&oven_mutex);
    i = 0;
    int ovenid = 0;
    while (i < num_ovens)
    {
        if (oven_ptr[i] == false)
        {
            // oven_ptr[i] = true; // CAN CAUSE DEADLOCK
            ovenid = i;
            goto chef1;
        }
        i++;
    }
    ////*******************************
    // HANDLE THE CASE WHEN OVN DOESN'T GET ALLOCATED//
    ///******************************
    pthread_cond_wait(&chef_cond_for_oven, &oven_mutex);
    i = 0;
    while (i < num_ovens)
    {
        if (oven_ptr[i] == false)
        {
            // oven_ptr[i] = true; // CAN CAUSE DEADLOCK
            ovenid = i;
        }
        i++;
    }
    pthread_mutex_unlock(&oven_mutex);

chef1:;
    blue();
    printf("Chef %d has put the pizza %d for order %d in oven %d.\n", id, pizza_id, order_id, ovenid);
    reset();
    int sleep_time = MAX(pizza_ptr[pizza_id].prep_time - 3, 0);
    sleep(sleep_time);
    blue();
    printf("Chef %d has left the pizza %d for order %d from the oven %d.\n", id, pizza_id, order_id, ovenid);
    reset();

    pthread_mutex_lock(&oven_mutex);
    oven_ptr[ovenid] = 0;
    pthread_mutex_lock(&oven_mutex);

    order_ptr[order_id].new_pizza = pizza_id;
    pthread_mutex_lock(&(order_ptr[order_id].order_mutex));
    order_ptr[order_id].num_pizzas_made += 1;
    pthread_mutex_unlock(&(order_ptr[order_id].order_mutex));

end:;
    if (rejected_due_to_time == true)
    {
        pthread_mutex_lock(&pizza_queue);
        enqueue(pizza_id, order_id);
        pthread_mutex_lock(&pizza_queue);
    }
    else if (rejected_due_to_time == false)
    {
        pthread_mutex_lock(&(order_ptr[order_id].order_mutex));
        order_ptr[order_id].num_pizzas_processed += 1;
        if (order_ptr[order_id].num_pizzas_processed == order_ptr[order_id].num_pizzas)
        {
            pthread_cond_signal(&(order_ptr[order_id].order_cond));
        }
        pthread_mutex_unlock(&(order_ptr[order_id].order_mutex));
    }
    pthread_cond_signal(&chef_cond_for_oven);
    pthread_mutex_unlock(&(chef_ptr[id].chef_mutex));

    clock_gettime(CLOCK_REALTIME, &ts);
    curr_time = ts.tv_sec - globaltime;
    if (curr_time <= chef_ptr[id].exit_time)
    {
        goto start;
    }

    pthread_mutex_lock(&restaurant_check);
    chefs_working -= 1;
    activeChefs -= 1;
    if (chefs_working == 0)
    {
        int j = 0;
        while (j < num_customers)
        {
            pthread_cond_signal(&(order_ptr[j].order_cond));
            j++;
        }
    }
    blue();
    printf("Chef %d exits at time %d.\n", id, curr_time - chef_ptr[id].arr_time);
    reset();
    pthread_mutex_unlock(&restaurant_check);
    // return NULL;
}

void *orderinit(void *args)
{
    int id = *((int *)args);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int curr_time = ts.tv_sec - globaltime;
    sleep(order_ptr[id].arr_time - curr_time);

    clock_gettime(CLOCK_REALTIME, &ts);
    curr_time = ts.tv_sec;
    yellow();
    printf("Customer %d arrives at time %d\n", id, curr_time - globaltime);
    reset();

    pthread_mutex_lock(&drive_thru);
    if (drive_thru_customers >= drive_thru_max_customers)
    {
        while (drive_thru_customers >= drive_thru_max_customers)
            pthread_cond_wait(&drive_next, &drive_thru);
    }
    else
    {
        drive_thru_customers++;
    }
    pthread_mutex_unlock(&drive_thru);

    pthread_mutex_lock(&chefs_check);
    if (chefs_working > 0)
        pthread_mutex_unlock(&chefs_check);
    else
    {
        red();
        printf("Customer %d rejected.\n", id);
        printf("Customer %d exits the drive-thru zone.\n", id);
        reset();
        pthread_mutex_unlock(&chefs_check);
        return NULL;
    }
    pthread_mutex_unlock(&chefs_check);

    curr_time = ts.tv_sec;
    int i = 0;

    pthread_mutex_lock(&(order_ptr[id].order_mutex));
    red();
    printf("Order %d placed by customer %d has pizzas {", id, id);
    while (i < order_ptr[id].num_pizzas)
    {
        if (i == order_ptr[id].num_pizzas - 1)
            printf("%d}.\n", order_ptr[id].pizzas[i].id);
        else
            printf("%d, ", order_ptr[id].pizzas[i].id);
        order_ptr[id].status = 3; // accepted
        i++;
    }
    i = 0;

    pthread_mutex_lock(&pizza_queue);
    while (i < order_ptr[id].num_pizzas)
    {
        enqueue(order_ptr[id].pizzas[i].id - 1, id);
        i++;
    }
    pthread_mutex_unlock(&pizza_queue);

    red();
    printf("Order %d placed by customer %d awaits processing.\n", id, id);
    reset();

    // now the driver will take some time to reach pickup point
    sleep(reach_pickup_time);

    // we have reached pickup point, hence one car from the drive thru has cleared
    pthread_mutex_lock(&drive_thru);
    drive_thru_customers--;
    if (drive_thru_customers >= drive_thru_max_customers)
    {
        ;
    }
    else
    {
        pthread_cond_signal(&drive_next);
    }
    pthread_mutex_unlock(&drive_thru);

    pthread_mutex_lock(&restaurant_check);
    if (chefs_working != 0)
    {
        pthread_mutex_unlock(&restaurant_check);
    }
    else
    {
        pthread_mutex_unlock(&(order_ptr[id].order_mutex));
        red();
        printf("Customer %d rejected.\n", id);
        printf("Customer %d exits the drive-thru zone.\n", id);
        reset();
        pthread_mutex_unlock(&restaurant_check);
        return NULL;
    }
    pthread_mutex_unlock(&restaurant_check);

    while (chefs_working > 0 && order_ptr[id].num_pizzas_processed < order_ptr[id].num_pizzas)
    {
        pthread_cond_broadcast(&chef_cond);
        pthread_cond_wait(&(order_ptr[id].order_cond), &(order_ptr[id].order_mutex));
    }

    pthread_mutex_lock(&restaurant_check);
    if (chefs_working > 0)
    {
        pthread_mutex_unlock(&restaurant_check);
    }
    else
    {
        yellow();
        printf("Customer %d exits the drive-thru.\n", id);
        reset();

        pthread_mutex_unlock(&(order_ptr[id].order_mutex));
        pthread_mutex_unlock(&restaurant_check);
        return NULL;
    }
    pthread_mutex_unlock(&restaurant_check);

    int check = check_order_status(id);
    if (check == 1)
    {
        int x = order_ptr[id].status;
        if (x == 4)
        { // rejected
            pthread_mutex_unlock(&(order_ptr[id].order_mutex));
            yellow();
            printf("Customer %d is rejected.\n", id);
            printf("Customer %d exits the drive-thru zone.\n", id);
            reset();

            return NULL;
        }
        else if (x == 5)
        { // partially cooked
            red();
            printf("Order %d placed by customer %d partially processed and remaining couldn't be.\n", id, id);
            reset();
        }
        else if (x == 6)
        { // cooked
            red();
            printf("Order %d placed by customer %d has been processed.\n", id, id);
            reset();
        }
    }

    yellow();
    printf("Customer %d exits the drive-thru.\n", id);
    reset();

    pthread_mutex_unlock(&(order_ptr[id].order_mutex));

    return NULL;
}

void sem_initialisation()
{
    pthread_mutex_init(&pizza_queue, NULL);
    pthread_mutex_init(&drive_thru, NULL);
    pthread_mutex_init(&chefs_check, NULL);
    pthread_mutex_init(&restaurant_check, NULL);
    pthread_mutex_init(&ingredient_lock, NULL);
    pthread_mutex_init(&oven_mutex, NULL);

    pthread_cond_init(&drive_next, NULL);
    pthread_cond_init(&chef_cond, NULL);
    pthread_cond_init(&chef_cond_for_oven, NULL);
}

int main()
{
    srand(time(NULL));
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    globaltime = ts.tv_sec;

    get_input();
    sem_initialisation();
    int i = 0;
    while (i < num_customers)
    {
        pthread_mutex_init(&(order_ptr[i].order_mutex), NULL);
        order_ptr[i].thread_id = pthread_create(&(order_ptr[i].order_thread), NULL, orderinit, (void *)(&(order_ptr[i].id)));
        i++;
    }
    i = 0;
    while (i < num_chefs)
    {
        pthread_mutex_init(&(chef_ptr[i].chef_mutex), NULL);
        chef_ptr[i].thread_id = pthread_create(&(chef_ptr[i].chef_thread), NULL, orderinit, (void *)(&(chef_ptr[i].id)));
        i++;
    }
    i = 0;
    while (i < num_customers)
    {
        pthread_join(order_ptr[i].order_thread, NULL);
    }
    i = 0;
    while (i < num_chefs)
    {
        pthread_join(chef_ptr[i].chef_thread, NULL);
    }
    return 0;
}
