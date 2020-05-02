#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <queue>
#include <time.h>
#include <sys/time.h>

typedef unsigned long planeid_t;
typedef struct plane
{
    planeid_t id;
    char status;
    struct timeval rt;
    struct timeval rnt;
} plane;

pthread_cond_t pcond_landing, pcond_departure;
pthread_mutex_t lock;

int simulation_time, sleep_time, count, probabilty, order;
struct timeval now, init;

std::queue<plane> landing_queue, departure_queue, emergency_queue;
std::queue<planeid_t> landing_queue_strs, departure_queue_strs;

void *landing(void *id);

void *departure(void *id);

void *action(void *act);

void print_queues();

void destroy_threads();

int pthread_sleep(int seconds);

inline void log_print(plane p);

inline void log_title();

int main(int argc, char *argv[])
{
    // Default values for variables.
    simulation_time = 200;
    sleep_time = 2;
    probabilty = 50;
    order = 0;
    id_t plane_id = 0;

    // Initializations
    pthread_cond_init(&pcond_landing, NULL);
    pthread_cond_init(&pcond_departure, NULL);
    pthread_mutex_init(&lock, NULL);
    gettimeofday(&init, NULL);
    gettimeofday(&now, NULL);

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-s") == 0) // Get simulation time if entered.
        {
            simulation_time = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-p") == 0) // Get probability if entered.
        {
            probabilty = atoi(argv[i]) * 100;
        }
    }

    // Keeping track of threads.
    int thread_count = simulation_time / sleep_time;
    pthread_t *threads = new pthread_t[thread_count];

    // Creating main thread.
    pthread_t main_thread;
    pthread_create(&main_thread, NULL, action, (void *)NULL);

    // Main Loop for simulation.
    while (now.tv_sec <= init.tv_sec + simulation_time)
    {
        printf("Current time: %d\n", now.tv_sec);

        // Creating random probability to decide landing or departure of the plane.
        srand(time(NULL));
        int random_probabilty = rand() % 100;

        // ??????????????????????????????????
        if ((now.tv_sec - init.tv_sec) % (2 * sleep_time) == 0)
            count++;

        // Emergency Landing
        if ((now.tv_sec - init.tv_sec) / sleep_time == 40)
        {
            pthread_mutex_lock(&lock);
            plane p;
            p.rt = now;
            emergency_queue.push(p);
            plane_id++;

            if ((now.tv_sec - init.tv_sec) >= order)
                print_queues();

            pthread_mutex_unlock(&lock);
        }

        // Landing
        else if (random_probabilty <= probabilty)
        {
            printf("Plane %d is landing.\n", plane_id);
            pthread_create(&threads[plane_id], NULL, landing, (void *)(intptr_t)plane_id);
            plane_id++;
        }

        // Departing
        else if (random_probabilty <= 100)
        {
            printf("Plane %d is departing.\n", plane_id);
            pthread_create(&threads[plane_id], NULL, departure, (void *)(intptr_t)plane_id);
            plane_id++;
        }
        else
        {
            pthread_mutex_lock(&lock);
            if ((now.tv_sec - init.tv_sec) >= order)
                print_queues();

            pthread_mutex_unlock(&lock);
        }

        pthread_sleep(sleep_time);
        gettimeofday(&now, NULL);
    }

    destroy_threads();
    delete[] threads;
    return 0;
}

void *action(void *act)
{
    log_title();
    while (now.tv_sec <= init.tv_sec + simulation_time)
    {
        pthread_mutex_lock(&lock);

        // Emergency Landing
        if (!emergency_queue.empty())
        {
            printf("Emergency landing has been occured.\n");
            emergency_queue.pop();
        }

        // Landing
        // TODO: DEFINE CONDITIONS
        else if (!landing_queue.empty() && !(!departure_queue.empty() && ((now.tv_sec - departure_queue.front().rt.tv_sec) >= 10 * sleep_time))
                 || landing_queue.size() > departure_queue.size() // ******* Prevent landing starvation by disallowing existence of a longer landing queue than departure queue ********
                )
        {
            plane p = landing_queue.front();
            printf("Plane %d is notifying the tower to land. \n", p.id);
            landing_queue.pop();
            landing_queue_strs.pop();
            pthread_cond_signal(&pcond_landing);
        }

        // Departing
        // TODO: DEFINE CONDITIONS
        else if (!departure_queue.empty())
        {
            plane p = departure_queue.front();
            printf("Plane %d is notifying the tower to depart. \n", p.id);
            departure_queue.pop();
            departure_queue_strs.pop();
            pthread_cond_signal(&pcond_departure);
        }

        pthread_mutex_unlock(&lock);
        pthread_sleep(2 * sleep_time);
    }
    pthread_exit(NULL);
}

void *landing(void *id)
{
    plane p;
    p.id = (planeid_t)id;
    gettimeofday(&(p.rt), NULL);
    p.status = 'L';
    pthread_mutex_lock(&lock);
    //printf("I locked for land: %d\n\n", p.id);
    //printf("Pushing to queue %d \n", p.id);
    landing_queue.push(p);
    landing_queue_strs.push(p.id);
    //  printf("Pushed to queue to land %d --  \n\n", p.id);//, lq.back().id);
    if ((now.tv_sec - init.tv_sec) >= order)
        print_queues();

    pthread_cond_wait(&pcond_landing, &lock);
    gettimeofday(&(p.rnt), NULL);
    //printf("here\n");
    log_print(p);

    pthread_mutex_unlock(&lock);
    // pthread_cond_init(&(p.pcond), NULL);
    //pthread_cond_wait(&(p.pcond), &plock);
    //printf("Eheeey: %ld\n\n", p.id);
    //pthread_cond_destroy(&(p.pcond));

    pthread_exit(NULL);
}

void *departing(void *id)
{
    plane p;
    p.id = (planeid_t)id;
    gettimeofday(&(p.rt), NULL);
    p.status = 'D';
    pthread_mutex_lock(&lock);
    if (tdelta >= order)
        print_queues();
    //printf("I locked depart: %d\n\n", p.id);
    departure_queue.push(p);
    departure_queue_strs.push(p.id);
    //printf("Pushed to queue depart  %d --  \n\n", p.id);//lq.back().id);
    //cout << dq_strs << "\n";

    pthread_cond_wait(&pcond_departure, &lock);
    gettimeofday(&(p.rnt), NULL);
    //  printf("there\n");
    log_print(p);
    //  printf("Pushing to queue %d \n\n", p.id);
    //pthread_cond_init(&(p.pcond), NULL);
    //pthread_cond_wait(&(p.pcond), &plock);
    //printf("Alooha: %ld\n\n", p.id);
    //pthread_cond_destroy(&(p.pcond));
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

// Destroys landing and departure pthread condition variables and mutex lock.
void destroy_threads()
{
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&pcond_landing);
    pthread_cond_destroy(&pcond_departure);
}

// Prints plane id's in landing and departure queues.
void print_queues()
{
    printf("\nLanding Queue: ");
    for (int i = 0; i < landing_queue.size(); i++)
    {
        planeid_t id = landing_queue_strs.front();
        printf("%d,", id);
        landing_queue_strs.pop();
        landing_queue_strs.push(id);
    }

    printf("\nDeparture Queue: ");
    for (int i = 0; i < departure_queue.size(); i++)
    {
        planeid_t id = departure_queue_strs.front();
        printf("%d,", id);
        departure_queue_strs.pop();
        departure_queue_strs.push(id);
    }
    printf("\n");
}

inline void log_title()
{
    printf("%20s %20s %20s %20s %20s\n%s\n", "PlaneID", "Status",
           "Request Time", "Runway Time", "Turnaround Time",
           "---------------------------------------------------------------------------------------------------------------------------");
}

inline void log_print(plane p)
{
    printf("%20d %20c %20d %20d %20d\n", p.id, p.status, p.rt.tv_sec - init.tv_sec,
                                        p.rnt.tv_sec - init.tv_sec, p.rnt.tv_sec - p.rt.tv_sec);
}

/******************************************************************************
  pthread_sleep takes an integer number of seconds to pause the current thread
  original by Yingwu Zhu
  updated by Muhammed Nufail Farooqi
 *****************************************************************************/
int pthread_sleep(int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if (pthread_mutex_init(&mutex, NULL))
    {
        return -1;
    }
    if (pthread_cond_init(&conditionvar, NULL))
    {
        return -1;
    }
    struct timeval tp;
    //When to expire is an absolute time, so get the current time and add //it to our delay time
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds;
    timetoexpire.tv_nsec = tp.tv_usec * 1000;

    pthread_mutex_lock(&mutex);
    int res = pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);

    //Upon successful completion, a value of zero shall be returned
    return res;
}