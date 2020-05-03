# COMP304-Air-Traffic-Control

# Project 2 

### I've implemented all parts of the project. 
However there is a litte bug in my project. As time goes by, planes are accumulating in queue and in somewhere when a plane try to take off program locked. Because of this lock my log file cannot be created at the end bu you can find my lod in the console. 

I have included a Makefile with my project.
It can be used to compile the code with the command in directory: ``` make ```
To run the program with predefined arguments, you can use "make run"
To run the program with custom arguments, specify them in the following format:

``` 
./planes.o -s 100 -p 50
```

Our program starts with main, creates air traffic control thread and landing or departing thread for each plane.
The main thread stays alive creates planes based on random probability.
In the mean time each plane thread logs its data to log file.
Air traffic control thread looks up queues and gives permission to land or take off based on defined conditions. I define a maximum wait time for planes which are waiting to take off and if landing queue's size is larger than departing queue, traffic control allow planes to land in order to prevent landing queue from starvation.

``` 
!(!departure_queue.empty() && ((now.tv_sec - departure_queue.front().request_time.tv_sec) >= 10 * sleep_time)) // Max waiting time.
|| landing_queue.size() > landing_queue.size() // Prevents landing starvation
```
When the each plane therad terminates, related thread  returns with pthread_exit(). When simulation ends main destroys all threads and conditions.

I've used 1 mutex lock to make sure there is no collusion in the runway and protect data.

### We created two log samples with using

