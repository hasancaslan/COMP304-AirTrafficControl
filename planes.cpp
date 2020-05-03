#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstdarg>
#include <pthread.h>
#include <queue>
#include <ctime>
#include <sys/time.h>

typedef unsigned long plane_id_t;

enum plane_status_t
{
    LANDING = 'L',
    DEPARTING = 'D',
    EMERGENCY = 'E',
    UNKNOWN
};

typedef struct plane
{
    plane_id_t id;
    plane_status_t status;
    struct timeval request_time;
    struct timeval runway_time;
    //pthread_cond_t pcond;
    //pthread_mutex_t plock;
} plane;

FILE *planes_log;

pthread_cond_t pcond_landing, pcond_departure;
pthread_mutex_t lock;

int simulation_time, sleep_time, probabilty, order;
struct timeval now, init;

std::queue<plane> landing_queue, departure_queue, emergency_queue;
std::queue<plane_id_t> landing_queue_str, departure_queue_str;

void *landing(void *id);

void *departing(void *id);

void *air_traffic_control(void *atc);

void print_queues();

void destroy_threads();

int pthread_sleep(int seconds);

int main(int argc, char *argv[])
{
    // Default values for variables.
    simulation_time = 200;
    sleep_time = 1;
    probabilty = 50;
    order = 0;
    id_t landing_id = 0;
    id_t departure_id = 1;

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
            simulation_time = atoi(argv[i+1]);
        }
        else if (strcmp(argv[i], "-p") == 0) // Get probability if entered.
        {
            probabilty = atof(argv[i+1]) * 100;
        }
        else if (strcmp(argv[i], "-n") == 0) // Starting from nth secs to terminal.
        {
            order = atoi(argv[i+1]);
        }
    }

    // Creating log file.
    char control_log_file[21];
    snprintf(control_log_file, 21, "%02ld-planes.log", init.tv_sec);
    planes_log = fopen(control_log_file, "w");

    // Keeping track of threads.
    int thread_count = simulation_time / sleep_time;
    pthread_t *threads = new pthread_t[thread_count];

    // Creating air traffic control thread.
    pthread_t air_traffic_control_thread;
    pthread_create(&air_traffic_control_thread, NULL, air_traffic_control, (void *)NULL);

    printf("Plane %d is ready to land.\n", landing_id);
    pthread_create(&threads[landing_id], NULL, landing, (void *)(intptr_t)landing_id);
    landing_id += 2;

    printf("Plane %d is ready to depart.\n", departure_id);
    pthread_create(&threads[departure_id], NULL, departing, (void *)(intptr_t)departure_id);
    departure_id += 2;

    // Main Loop for simulation.
    while (now.tv_sec <= init.tv_sec + simulation_time)
    {
        // Creating random probability to decide landing or departure of the plane.
        srand(time(NULL));
        int random_probabilty = rand() % 100;

        // Emergency Landing
        if ((now.tv_sec - init.tv_sec) / sleep_time == 40)
        {
            pthread_mutex_lock(&lock);
            plane p;
            p.status = EMERGENCY;
            p.request_time = now;
            emergency_queue.push(p);
            landing_id += 2;

            if ((now.tv_sec - init.tv_sec) >= order)
                print_queues();

            pthread_mutex_unlock(&lock);
        }

            // Landing
        else if (random_probabilty <= probabilty)
        {
            printf("Plane %d is ready to land.\n", landing_id);
            pthread_create(&threads[landing_id], NULL, landing, (void *)(intptr_t)landing_id);
            landing_id += 2;
        }

            // Departing
        else if (random_probabilty <= 100)
        {
            printf("Plane %d is ready to depart.\n", departure_id);
            pthread_create(&threads[departure_id], NULL, departing, (void *)(intptr_t)departure_id);
            departure_id += 2;
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
    fclose(planes_log);
    delete[] threads;
    return 0;
}

// Function for writing logs to file.
void log_file(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(planes_log, format, args);
    va_end(args);
}

// Logs column's titles.
void log_column_titles()
{
    log_file("%20s %20s %20s %20s %20s\n%s\n", "PlaneID", "Status",
             "Request Time", "Runway Time", "Turnaround Time",
             "___________________________________________________________________________________________________________________________");
}

// Logs planes data to file with specified format.
void log_plane(plane p)
{
    log_file("%20lu %20c %20lu %20lu %20lu\n", p.id, p.status, p.request_time.tv_sec - init.tv_sec,
             p.runway_time.tv_sec - init.tv_sec, p.runway_time.tv_sec - p.request_time.tv_sec);
}

// Prints plane id's in landing and departure queues.
void print_queues()
{
    printf("\nPlanes on the Air: ");
    for (int i = 0; i < landing_queue.size(); i++)
    {
        plane_id_t id = landing_queue_str.front();
        printf("%lu - ", id);
        landing_queue_str.pop();
        landing_queue_str.push(id);
    }

    printf("\nPlanes on the Ground: ");
    for (int i = 0; i < departure_queue.size(); i++)
    {
        plane_id_t id = departure_queue_str.front();
        printf("%lu - ", id);
        departure_queue_str.pop();
        departure_queue_str.push(id);
    }
    printf("\n");
}

void *air_traffic_control(void *atc)
{
    log_column_titles();
    while (now.tv_sec <= init.tv_sec + simulation_time)
    {
        pthread_mutex_lock(&lock);

        // Emergency Landing
        if (!emergency_queue.empty())
        {
            plane p = landing_queue.front();
            printf("Emergency landing for plane %lu. \n", p.id);
            emergency_queue.pop();
            pthread_cond_signal(&pcond_landing);
        }

            // Landing
        else if (!landing_queue.empty()                                                                                            // No more planes is waiting to land.
                 && departure_queue.size() < 5                                                                                     // 5 planes or more lined up to take off.
                 && !(!departure_queue.empty() && ((now.tv_sec - departure_queue.front().request_time.tv_sec) >= 20 * sleep_time)) // Max waiting time.
                 || landing_queue.size() > departure_queue.size()                                                                  // Prevents landing starvation
                )
        {
            plane p = landing_queue.front();
            printf("Plane %lu is landing. \n", p.id);
            landing_queue.pop();
            landing_queue_str.pop();
            pthread_cond_signal(&pcond_landing);
        }

            // Departing
        else if (!departure_queue.empty())
        {
            plane p = departure_queue.front();
            printf("Plane %lu is departing. \n", p.id);
            departure_queue.pop();
            departure_queue_str.pop();
            pthread_cond_signal(&pcond_departure);
        }

        pthread_mutex_unlock(&lock);
        pthread_sleep(2 * sleep_time);
    }
    pthread_exit(NULL);
}

// Landing thread function.
void *landing(void *id)
{
    plane p;
    p.id = (plane_id_t)id;
    p.status = LANDING;
    gettimeofday(&(p.request_time), NULL);

    pthread_mutex_lock(&lock);
    landing_queue.push(p);
    landing_queue_str.push(p.id);
    if ((now.tv_sec - init.tv_sec) >= order)
    {
        print_queues();
    }

    pthread_cond_wait(&pcond_landing, &lock);
    gettimeofday(&(p.runway_time), NULL);
    log_plane(p);
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

// Departure thread function.
void *departing(void *id)
{
    plane p;
    p.id = (plane_id_t)id;
    p.status = DEPARTING;
    gettimeofday(&(p.request_time), NULL);

    pthread_mutex_lock(&lock);
    if ((now.tv_sec - init.tv_sec) >= order)
    {
        print_queues();
    }
    departure_queue.push(p);
    departure_queue_str.push(p.id);

    pthread_cond_wait(&pcond_departure, &lock);
    gettimeofday(&(p.runway_time), NULL);
    log_plane(p);
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