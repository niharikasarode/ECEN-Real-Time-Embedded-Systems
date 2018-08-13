/* ========================================================================== */
/*                                                                            */
// Sam Siewert, December 2017
//
// Sequencer Generic
//
// The purpose of this code is to provide an example for how to best
// sequence a set of periodic services for problems similar to and including
// the final project in real-time systems.

//define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <stdbool.h>
#include <semaphore.h>
#include <syslog.h>
#include <sys/time.h>
#include <errno.h>
#include <bits/stdc++.h>

#include <iostream>       // std::cout
#include <iomanip>        // std::put_time
#include <thread>         // std::this_thread::sleep_until
#include <chrono>         // std::chrono::system_clock
#include <ctime>          // std::time_t, std::tm, std::localtime, std::mktime

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#define DEBUG_PRINT 0
#define USEC_PER_MSEC (1000)
#define NANOSEC_PER_SEC (1000000000)
#define NUM_CPU_CORES (1)
#define TRUE (1)
#define FALSE (0)
#define NUM_FRAMES 20

void print_scheduler(void);

struct timeval start_time_val;
int abortTest=FALSE;
char frame_contents[100*sizeof(char)];
//byte *frame_contents;
std::string SYS_DETAILS;

using namespace std;
using namespace cv;

VideoCapture capture(0);

pthread_mutex_t rsrc;
sem_t sem_capture, sem_ppm, sem_compress;

void *Start_capture(void *t)
{
	
	
	struct timeval current_time_val;
	static int img_cnt =0;
	int framebuf_sz = 0;
	char buf[5];
	char *fr_ptr;
	Mat frame;

	while(img_cnt <= NUM_FRAMES){
		
		sem_wait(&sem_capture);
		syslog(LOG_CRIT, "CAP S");//sem_wait(&sem_ppm);
		capture.open(0);
		capture >> frame;
		capture.release();
		
		fr_ptr = (char *)frame.data;
		//frame_contents = (char *)malloc(100*sizeof(char));
		pthread_mutex_lock(&rsrc);
		memcpy(frame_contents, &fr_ptr, sizeof(char *));
		pthread_mutex_unlock(&rsrc);
		/*framebuf_sz = frame.total()*frame.elemSize();
		frame_contents = new byte[framebuf_sz];
		std::memcpy(frame_contents, frame.data,framebuf_sz*sizeof(frame_contents));*/		
		printf("\n CHECK IMWRITE %d\n", img_cnt);
		//int n = sprintf(buf, "img%d.ppm", img_cnt);
		//imwrite(buf, frame);
		//cvSaveImage(buf, frame);
		gettimeofday(&current_time_val, (struct timezone *)0);
        	syslog(LOG_CRIT, "IMAGE %d captured at @ sec=%d, msec=%d\n", img_cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
		img_cnt++;
		syslog(LOG_CRIT, "CAP E");//sem_post(&sem_ppm);
		sem_post(&sem_ppm);
	}
	

}

void *PPMimg_formatter(void *t)
{
	
	struct timeval current_time_val;
	Mat ppm_frame;
	static int img_count=1;
	std::ostringstream fname;
	int t_s, t_ms;
	vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_PXM_BINARY);
	compression_params.push_back(1);
	char *fr_ptr;

	while(img_count <= NUM_FRAMES){
		sem_wait(&sem_ppm);
		syslog(LOG_CRIT, "PPM S");
		gettimeofday(&current_time_val, (struct timezone *)0);
        	syslog(LOG_CRIT, "PPMimg_formatter  at @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
		t_s = (int)(current_time_val.tv_sec-start_time_val.tv_sec);
		t_ms = (int)(current_time_val.tv_usec/USEC_PER_MSEC);
		/*Mat image = Mat(height, width, CV_8UC3, frame_contents);*/
		//fname << filename;
		fname << "PPM_" << img_count << ".ppm";
		pthread_mutex_lock(&rsrc);
		if(frame_contents) memcpy(&fr_ptr, frame_contents, sizeof(char *));
		pthread_mutex_unlock(&rsrc);
		ppm_frame = Mat(480, 640, CV_8UC3, fr_ptr);
		imwrite(fname.str(), ppm_frame, compression_params);
		
		std::fstream final;
		std::fstream temp;

		temp.open("new.txt", ios::in | ios::out | ios::trunc);
		final.open(fname.str(), ios::in | ios::out);
		final.seekp(ios::beg);
		final << "   ";
		temp << final.rdbuf();
		temp.close();
		final.close();
		temp.open("new.txt", ios::in | ios::out);
		final.open(fname.str(), ios::in | ios::out | ios::trunc);
		final << "P6" << endl << "#HOST: " << SYS_DETAILS << endl << "#TIMESTAMP: "<< t_s << "s" << t_ms << "ms" << endl <<temp.rdbuf();
		temp.close();
		final.close();
		img_count++;
		fname.str("");
		//system("sudo rm -r new.txt");
		syslog(LOG_CRIT, "PPM E");//free(frame_contents);
		sem_post(&sem_compress);
	}

}

void *Compress(void *t)
{
	
	struct timeval current_time_val;
	Mat jpg_frame;
	static int img_count=1;
	std::ostringstream filename;
	vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
	compression_params.push_back(95);
	char *fr_ptr;

	while(img_count <= NUM_FRAMES){
		sem_wait(&sem_compress);
		syslog(LOG_CRIT, "COMP S");/*Mat image = Mat(height, width, CV_8UC3, frame_contents);*/
		//printf("not supposed\n");
		filename << "JPG_" << img_count << ".jpg";
		pthread_mutex_lock(&rsrc);
		if(frame_contents) memcpy(&fr_ptr, frame_contents, sizeof(char *));

		jpg_frame = Mat(480, 640, CV_8UC3, fr_ptr);
		//printf("\n CHECK IMWRITE %d\n", img_count);
		imwrite(filename.str(), jpg_frame, compression_params);
		
		gettimeofday(&current_time_val, (struct timezone *)0);
        	syslog(LOG_CRIT, "JPEG Compress  at @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
		img_count++;
		filename.str("");
		memset(&frame_contents, 0, sizeof(frame_contents));
		syslog(LOG_CRIT, "COMP E");
		pthread_mutex_unlock(&rsrc);
	}

}

void *Sequencer(void *t)
{
    	struct timeval current_time_val;
    	struct timespec delay_time = {0,33333333}; // delay for 33.33 msec, 30 Hz
    	struct timespec remaining_time;
    	double current_time;
    	double residual;
    	int rc, delay_cnt=0;
    	unsigned long long seqCnt=0;
    	
	gettimeofday(&current_time_val, (struct timezone *)0);
    	syslog(LOG_CRIT, "Sequencer thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    	if(DEBUG_PRINT) printf("Sequencer thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

    	do
    	{
        	delay_cnt=0; residual=0.0;

        //gettimeofday(&current_time_val, (struct timezone *)0);
        //syslog(LOG_CRIT, "Sequencer thread prior to delay @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
        /*	do
        	{
            		rc=nanosleep(&delay_time, &remaining_time);

            		if(rc == EINTR)
            		{
                		residual = remaining_time.tv_sec + ((double)remaining_time.tv_nsec / (double)NANOSEC_PER_SEC);

                		if(residual > 0.0) 
				{
					if(DEBUG_PRINT) printf("residual=%lf, sec=%d, nsec=%d\n", residual, (int)remaining_time.tv_sec, (int)remaining_time.tv_nsec);
				}

            		}
            		else if(rc < 0)
            		{
				residual = remaining_time.tv_sec + ((double)remaining_time.tv_nsec / (double)NANOSEC_PER_SEC);

                		if(residual > 0.0) 
				{
					if(DEBUG_PRINT) printf("residual=%lf, sec=%d, nsec=%d\n", residual, (int)remaining_time.tv_sec, (int)remaining_time.tv_nsec);
				}
                		perror("Sequencer nanosleep");
                		//exit(-1);
            		}

        	} while(residual > 0.0);*/
		std::this_thread::sleep_for (std::chrono::milliseconds(33));
        	seqCnt++;
        	gettimeofday(&current_time_val, (struct timezone *)0);
        	syslog(LOG_CRIT, "Sequencer cycle %llu @ sec=%d, msec=%d\n", seqCnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
	if((seqCnt % 30) == 0) sem_post(&sem_capture);

	//if((seqCnt % 30) == 0) sem_post(&sem_ppm);

	//if((seqCnt % 30) == 0) sem_post(&sem_compress);
	}while(!abortTest);


}

int main(void)
{
	FILE *sys_details;
	char *buffer;
	buffer = (char *)malloc(30*sizeof(char));
	sys_details = popen("uname -a", "r");
	if (sys_details) {
    		while (!feof(sys_details))
    		if (fgets(buffer, 30, sys_details) != NULL) SYS_DETAILS.append(buffer);
    		pclose(sys_details);
    	}
	
	free(buffer);
	pthread_mutex_init(&rsrc, NULL);
    	struct timeval current_time_val;
    	int i, rc, scope;
    	cpu_set_t threadcpu;

	capture.set(CV_CAP_PROP_FRAME_HEIGHT, 640);
	capture.set(CV_CAP_PROP_FRAME_WIDTH, 480);

    	pthread_t sequencer, start_capture, ppmimg_formatter, compress_jpg;
    	pthread_attr_t main_attr, seq_attr, start_cap_attr, ppmimg_attr, compress_attr;
    	int rt_max_prio, rt_min_prio;
    	struct sched_param main_param, seq_p, start_capture_p, compress_p, ppmimg_p;
    	
    	pid_t mainpid;
    	cpu_set_t seqcpu;

	if (sem_init (&sem_capture, 0, 0)) { printf ("Failed to initialize sem_capture semaphore\n"); exit (-1); }
	if (sem_init (&sem_ppm, 0, 0)) { printf ("Failed to initialize sem_capture semaphore\n"); exit (-1); }
	//printf("Starting Sequencer Demo\n");
    	gettimeofday(&start_time_val, (struct timezone *)0);
    	gettimeofday(&current_time_val, (struct timezone *)0);
    	syslog(LOG_CRIT, "Sequencer @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

   	//adds exactly only one CPU to set

   	CPU_ZERO(&seqcpu);
	CPU_SET(0, &seqcpu);

   	if(CPU_ISSET(0, &seqcpu) > 0)
	{
		if(DEBUG_PRINT) printf("CPU 0 added to set\n");
	}

	mainpid=getpid();

    	rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    	rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    	rc=sched_getparam(mainpid, &main_param);
    	main_param.sched_priority=rt_max_prio;
    	rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
    	if(rc < 0) perror("main_param");
    	print_scheduler();
 	pthread_attr_getscope(&main_attr, &scope);

    	if(scope == PTHREAD_SCOPE_SYSTEM)
      		if(DEBUG_PRINT) printf("PTHREAD SCOPE SYSTEM\n");
    	else if (scope == PTHREAD_SCOPE_PROCESS)
      		if(DEBUG_PRINT) printf("PTHREAD SCOPE PROCESS\n");
    	else
      		if(DEBUG_PRINT) printf("PTHREAD SCOPE UNKNOWN\n");

    	printf("rt_max_prio=%d\n", rt_max_prio);
    	printf("rt_min_prio=%d\n", rt_min_prio);

	/*CPU_ZERO(&threadcpu);
      	CPU_SET(1, &threadcpu);
	if(CPU_ISSET(1, &threadcpu) > 0)
	{
		if(DEBUG_PRINT) printf("CPU 3 added to set\n");
	}*/

	rc=pthread_attr_init(&seq_attr);
      	rc=pthread_attr_setinheritsched(&seq_attr, PTHREAD_EXPLICIT_SCHED);
      	rc=pthread_attr_setschedpolicy(&seq_attr, SCHED_FIFO);
	seq_p.sched_priority=rt_max_prio;
	pthread_attr_setaffinity_np(&seq_attr, sizeof(cpu_set_t), &seqcpu);
    	pthread_attr_setschedparam(&seq_attr, &seq_p);
   
	
	rc=pthread_create(&sequencer, &seq_attr, Sequencer, (void *)0);
    	if(rc < 0)
        	perror("pthread_create for sequencer service 0");
    	else
        	printf("pthread_create successful for sequeencer service 0\n");

	rc=pthread_attr_init(&start_cap_attr);
      	rc=pthread_attr_setinheritsched(&start_cap_attr, PTHREAD_EXPLICIT_SCHED);
      	rc=pthread_attr_setschedpolicy(&start_cap_attr, SCHED_FIFO);
	start_capture_p.sched_priority=rt_max_prio-1;
	pthread_attr_setaffinity_np(&start_cap_attr, sizeof(cpu_set_t), &seqcpu);
    	pthread_attr_setschedparam(&start_cap_attr, &start_capture_p);

    	rc=pthread_create(&start_capture, &start_cap_attr,Start_capture,(void *)0);
    	if(rc < 0)
        	perror("pthread_create for start_capture");
    	else
        	printf("pthread_create successful for start_capture\n");

	rc=pthread_attr_init(&ppmimg_attr);
      	rc=pthread_attr_setinheritsched(&ppmimg_attr, PTHREAD_EXPLICIT_SCHED);
      	rc=pthread_attr_setschedpolicy(&ppmimg_attr, SCHED_FIFO);
	ppmimg_p.sched_priority=rt_max_prio-5;
	pthread_attr_setaffinity_np(&ppmimg_attr, sizeof(cpu_set_t), &seqcpu);
    	pthread_attr_setschedparam(&ppmimg_attr, &ppmimg_p);

    	rc=pthread_create(&ppmimg_formatter, &ppmimg_attr, PPMimg_formatter,(void *)0);
    	if(rc < 0)
        	perror("pthread_create for PPMimg_formatter");
    	else
        	printf("pthread_create successful for PPMimg_formatter\n");

	rc=pthread_attr_init(&compress_attr);
      	rc=pthread_attr_setinheritsched(&compress_attr, PTHREAD_EXPLICIT_SCHED);
      	rc=pthread_attr_setschedpolicy(&compress_attr, SCHED_FIFO);
	compress_p.sched_priority=rt_max_prio-9;
	pthread_attr_setaffinity_np(&compress_attr, sizeof(cpu_set_t), &seqcpu);
    	pthread_attr_setschedparam(&compress_attr, &compress_p);

    	rc=pthread_create(&compress_jpg, &compress_attr, Compress,(void *)0);
    	if(rc < 0)
        	perror("pthread_create for PPMimg_formatter");
    	else
        	printf("pthread_create successful for PPMimg_formatter\n");
	

	pthread_join(sequencer, NULL);
	pthread_join(start_capture, NULL);
	pthread_join(ppmimg_formatter, NULL);
	pthread_join(compress_jpg, NULL);

}

void print_scheduler(void)
{
   int schedType;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
       case SCHED_FIFO:
           printf("Pthread Policy is SCHED_FIFO\n");
           break;
       case SCHED_OTHER:
           printf("Pthread Policy is SCHED_OTHER\n"); exit(-1);
         break;
       case SCHED_RR:
           printf("Pthread Policy is SCHED_RR\n"); exit(-1);
           break;
       default:
           printf("Pthread Policy is UNKNOWN\n"); exit(-1);
   }
}
