#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <opencv2/opencv.hpp>
//#include <opencv2/tracking/tracker.hpp>
#include <opencv2/core/ocl.hpp>

using namespace cv;
using namespace std;

pthread_t cv_thread, update_thread, disp_thread;
pthread_attr_t cv_attr, update_attr, disp_attr, main_attr;

pthread_mutex_t storage_mutex;
sem_t update_sem, display_sem, cv_sem;

cpu_set_t affinity;

struct sched_param cv_param, disp_param, update_param, main_param;

void *cvthread(void*);
void *updatethread(void*);
void *dispthread(void*);

void *cvthread(void*)
{
	while(1)
	{
	sem_wait(&update_sem);
	sem_wait(&display_sem);
	printf("Into CV thread now\n");
	sem_post(&update_sem);
	}
}
void *updatethread(void*)
{
	while(1)
	{
	sem_wait(&cv_sem);
	printf("in update thread now\n");
	sem_post(&display_sem);
	}
}
void *dispthread(void*)
{
	while(1)
	{
	sem_wait(&update_sem);
	printf("In disp thread now\n");
	sem_post(&cv_sem);
	}
}
int main(int argc, char **argv)
{
	CPU_ZERO(&affinity);
	CPU_SET(0, &affinity);

	pthread_mutex_init(&storage_mutex, NULL);

	pthread_attr_init(&main_attr);
	pthread_attr_init(&cv_attr);
	pthread_attr_init(&update_attr);
	pthread_attr_init(&disp_attr);
	
	pthread_attr_setinheritsched(&cv_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&cv_attr, SCHED_FIFO);
	pthread_attr_setaffinity_np(&cv_attr, sizeof(cpu_set_t), &affinity);

	pthread_attr_setinheritsched(&update_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&update_attr, SCHED_FIFO);
	pthread_attr_setaffinity_np(&update_attr, sizeof(cpu_set_t), &affinity);


	pthread_attr_setinheritsched(&main_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&main_attr, SCHED_FIFO);
	pthread_attr_setaffinity_np(&main_attr, sizeof(cpu_set_t), &affinity);


	pthread_attr_setinheritsched(&disp_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&disp_attr, SCHED_FIFO);
	pthread_attr_setaffinity_np(&disp_attr, sizeof(cpu_set_t), &affinity);

	int max_prio = sched_get_priority_max(SCHED_FIFO);
	int min_prio = sched_get_priority_min(SCHED_FIFO);

	main_param.sched_priority = max_prio;
	cv_param.sched_priority = max_prio-1;
	update_param.sched_priority = max_prio-2;
	disp_param.sched_priority = max_prio-3;

	sem_init(&cv_sem, 0 , 1);
	sem_init(&update_sem, 0, 0);
	sem_init(&display_sem, 0, 0);

	int rc = sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
	if(rc)
	{
		perror(NULL);
		printf("Error setting scheduler: %d\n", rc);		
		exit(1);
	}

	pthread_attr_setschedparam(&cv_attr, &cv_param);
	pthread_attr_setschedparam(&update_attr, &update_param);
	pthread_attr_setschedparam(&disp_attr, &disp_param);
	pthread_attr_setschedparam(&main_attr, &main_param);

	sem_post(&cv_sem);

	if(pthread_create(&cv_thread, &cv_attr, cvthread, (void *)0))
	{
		printf("Error creating CV thread\n");
		perror(NULL);		
		exit(1);
	}
	
	if(pthread_create(&update_thread, &update_attr, updatethread, (void *)0))
	{
		printf("Error creating  update thread\n");
		perror(NULL);
		exit(1);
	}
	
	if(pthread_create(&disp_thread, &disp_attr, dispthread, (void *)0))
	{
		printf("Error creating disp thread\n");
		perror(NULL);
		exit(1);
	}
	
	pthread_join(cv_thread, NULL);
	pthread_join(update_thread, NULL);
	pthread_join(disp_thread, NULL);

return 0;
}
