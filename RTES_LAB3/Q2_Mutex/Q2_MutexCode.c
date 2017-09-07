#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define update_arg 12.34567

void *thread_update();
void *thread_read();

int i;

typedef struct{
double X_acc;
double Y_acc;
double Z_acc;
double Roll;
double Pitch;
double Yaw;
struct timespec Sample_time;

}Space_Param1;

Space_Param1 Space_Param;



pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

void *thread_update()
{
        int result;
        Space_Param1 Space_Param_Update;

        Space_Param_Update.X_acc = update_arg;
        Space_Param_Update.Y_acc = update_arg + 1.005;
        Space_Param_Update.Z_acc = update_arg + 2.005;
        Space_Param_Update.Roll = update_arg + 3.005;
        Space_Param_Update.Pitch = update_arg + 4.005;
        Space_Param_Update.Yaw = update_arg + 5.005;

        result = clock_gettime(CLOCK_REALTIME, &Space_Param_Update.Sample_time);
        if(result != 0)
       {
         perror("getclock");
       }
        /* printf(" Clock gettime function status 1 : %d, clock_tvsec : %lld, clock+tvnsec : %lf.\n", result, Space_Param_Update.Sample_time.tv_sec, Space_Param_Update.Sample_time.tv_nsec);
        */
        Space_Param_Update.Sample_time.tv_sec += 50;
        Space_Param_Update.Sample_time.tv_nsec += 1000000;

        pthread_mutex_lock(&mutex1);

        Space_Param = Space_Param_Update;
        /*printf ("1 X : %lf, Y : %lf , Z : %lf, Roll : %lf,Pitch : %lf, Yaw : %lf\n", Space_Param.X_acc, Space_Param.Y_acc, Space_Param.Z_acc,         Space_Param.Roll,Space_Param.Pitch, Space_Param.Yaw);
        */
        result = clock_settime(CLOCK_REALTIME, &Space_Param.Sample_time);
        if(result != 0)
        perror("setclock");
        /*printf(" Clock settime function status 2 : %d, clock_tvsec : %lld, clock+tvnsec : %lf.\n", result, Space_Param.Sample_time.tv_sec, Space_Param.Sample_time.tv_nsec);
        */
        pthread_mutex_unlock(&mutex1);

}

void *thread_read()
{

        Space_Param1 Space_Param_Read;
        int result;

        pthread_mutex_lock(&mutex1);

        Space_Param_Read = Space_Param;
     
        
        printf (" R1 X : %lf, Y : %lf , Z : %lf, Roll : %lf,Pitch : %lf, Yaw : %lf.\n", Space_Param_Read.X_acc, Space_Param_Read.Y_acc, Space_Param_Read.Z_acc, Space_Param_Read.Roll,Space_Param_Read.Pitch, Space_Param_Read.Yaw);
        
        result = clock_gettime(CLOCK_REALTIME, &Space_Param_Read.Sample_time);
        if(result != 0)
        perror("getclock");
        if(i == 1)
        {
        printf(" R2 Clock gettime function status : %d, clock_tvsec : %llf, clock+tvnsec : %lf.\n", result, Space_Param_Read.Sample_time.tv_sec, Space_Param_Read.Sample_time.tv_nsec);
        }


        pthread_mutex_unlock(&mutex1);


}






int main()
{

        pthread_t thread1, thread2;
        int rc1, rc2;

        if(pthread_mutex_init(&mutex1, NULL) != 0)
        {
                perror("mutex_init error");
        }

        for(i=0; i<3; i++)
        {

                if((rc1=pthread_create(&thread1, NULL, &thread_update, NULL)) != 0 )
                {
                        perror("thread1_create error");
                }

                if((rc2=pthread_create(&thread2, NULL, &thread_read, NULL)) != 0 )
                {
                        perror("thread1_create error");
                }
        }
        usleep(100000);
        pthread_join( thread1, NULL);
        pthread_join( thread2, NULL);

}

