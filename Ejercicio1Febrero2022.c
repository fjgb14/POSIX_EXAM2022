#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>

#define Prioridad_Nitrato1 25
#define Prioridad_Nitrato2 24
#define Prioridad_Monitor 23

struct Nitrato1 {
    int salinidad;
    int alcalinidad;
    int nitrato1;
    pthread_mutex_t mutex;
};

struct Nitrato2 {
    int o2;
    int pH;
    int nitrato2;
    pthread_mutex_t mutex;
};

struct Monitor {
    struct Nitrato1 nitrato1;
    struct Nitrato2 nitrato2;
};

void addtime(struct timespec *ts, long ms){
    ts->tv_nsec += ms * 1000000L;
    ts->tv_sec += ts->tv_nsec / 1000000000L;
    ts->tv_nsec %= 1000000000L;
};

void *tarea1(void *arg){
    
    struct Nitrato1 *nitrato1 = (struct Nitrato1*)arg;
    struct timespec siguiente;

    int valor_total,alcalinidad,salinidad;
    
    clock_gettime(CLOCK_MONOTONIC,&siguiente);
    
    while(1){
        pthread_mutex_lock(&nitrato1->mutex);
        alcalinidad = rand()%101;
        salinidad = rand()%101;
        valor_total =  alcalinidad*0.35 + salinidad*0.65;
        nitrato1->alcalinidad = alcalinidad;
        nitrato1->salinidad = salinidad;
        nitrato1->nitrato1 = valor_total;
        printf("Tarea1: He realizado una estimacion del nitrato.\n");
        pthread_mutex_unlock(&nitrato1->mutex);

        addtime(&siguiente,1000);
        clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME, &siguiente,NULL);
    }
 }

 void *tarea2(void *arg){

    struct Nitrato2 *nitrato2 = (struct Nitrato2*)arg;
    struct timespec siguiente;

    int valor_total,ph,o2;
    
    clock_gettime(CLOCK_MONOTONIC,&siguiente);

    while(1){
        pthread_mutex_lock(&nitrato2->mutex);
        ph = rand()%101;
        o2 = rand()%101;
        valor_total = 0.8*ph + 0.2*o2;
        nitrato2->pH = ph;
        nitrato2->o2 = o2;
        nitrato2->nitrato2 = valor_total;
        printf("Tarea2: He realizado una estimacion del nitrato.\n");
        pthread_mutex_unlock(&nitrato2->mutex);

        addtime(&siguiente,3000);
        clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&siguiente,NULL);
    }

 }

 void *tarea3(void *arg){
    struct Monitor *monitor=(struct Monitor*)arg;
    struct timespec siguiente;

    clock_gettime(CLOCK_MONOTONIC,&siguiente);

    while(1){
        pthread_mutex_lock(&monitor->nitrato1.mutex);
        pthread_mutex_lock(&monitor->nitrato2.mutex);
        printf("Tarea3:El valor del nitrato1 es: %d\n",monitor->nitrato1.nitrato1);
        printf("Tarea3:El valor del nitrato2 es: %d\n",monitor->nitrato2.nitrato2);
        pthread_mutex_unlock(&monitor->nitrato1.mutex);
        pthread_mutex_unlock(&monitor->nitrato2.mutex);

        addtime(&siguiente,4000);
        clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&siguiente,NULL);
    }

 }

 int main(){

    struct Monitor monitor;

    srand(time(NULL));
    mlockall(MCL_CURRENT | MCL_FUTURE);

    struct sched_param schedparam;
    schedparam.sched_priority = 50;
    pthread_setschedparam(pthread_self(), SCHED_RR, &schedparam);

    // -----------------

    pthread_mutexattr_t mutexattr;
    
    // Establecer techos de prioridad
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setprotocol(&mutexattr, PTHREAD_PRIO_PROTECT);

    pthread_mutexattr_setprioceiling(&mutexattr, Prioridad_Nitrato1);
    pthread_mutex_init(&monitor.nitrato1.mutex, &mutexattr);

    pthread_mutexattr_setprioceiling(&mutexattr, Prioridad_Nitrato2);
    pthread_mutex_init(&monitor.nitrato2.mutex, &mutexattr);

    // -----------------

    pthread_t th_tarea1, th_tarea2, th_monitor;
    pthread_attr_t th_attrs;

    pthread_attr_init(&th_attrs);
    pthread_attr_setinheritsched(&th_attrs, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&th_attrs, SCHED_RR);

    schedparam.sched_priority = Prioridad_Nitrato1;
    pthread_attr_setschedparam(&th_attrs, &schedparam);
    pthread_create(&th_tarea1, &th_attrs, tarea1, (void*)&monitor.nitrato1);

    schedparam.sched_priority = Prioridad_Nitrato2;
    pthread_attr_setschedparam(&th_attrs, &schedparam);
    pthread_create(&th_tarea2, &th_attrs, tarea2, (void*)&monitor.nitrato2);

    schedparam.sched_priority = Prioridad_Monitor;
    pthread_attr_setschedparam(&th_attrs, &schedparam);
    pthread_create(&th_monitor, &th_attrs, tarea3, (void*)&monitor);

    pthread_join(th_tarea1, NULL);
    pthread_join(th_tarea2, NULL);
    pthread_join(th_monitor, NULL);
    
    return 0;
 }