#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define SIG_CINTA_A (SIGRTMIN)
#define SIG_CINTA_B (SIGRTMIN + 1)
#define SIG_CINTA_A_NUEVA_MALETA (SIGRTMIN + 2)
#define SIG_CINTA_B_NUEVA_MALETA (SIGRTMIN + 3)
#define SIG_ENTRADA (SIGRTMIN + 4)
#define SIG_ENTRADA_CONTROL_A (SIGRTMIN + 5)
#define SIG_ENTRADA_CONTROL_B (SIGRTMIN +6)
#define POLITICA SCHED_RR

struct Almacen {
    pthread_mutex_t mutex;
    int maletasA;
    int maletasB;
};

void *cintaA(void* arg) {
    
    struct itimerspec its;
    its.it_interval.tv_sec = 2;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1;

    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIG_CINTA_A;

    timer_t timer;

    timer_create(CLOCK_MONOTONIC,&sigev,&timer);
    timer_settime(timer,0,&its,NULL);

    // -----------

    sigset_t mis_seniales;
    sigemptyset(&mis_seniales);
    sigaddset(&mis_seniales,SIG_CINTA_A);

    // -----------

    int info;
    while(1){
        sigwait(&mis_seniales,&info);
        if(info == SIG_CINTA_A){
            printf("Cinta A: avisando a control de entrada de que tengo una nueva maleta\n");
            sigval_t val;
            sigqueue(getpid(),SIG_CINTA_A_NUEVA_MALETA,val);
        }
    }

}

void *cintaB(void* arg) {
    
    struct itimerspec its;
    its.it_interval.tv_sec = 3;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1;

    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIG_CINTA_B;

    timer_t timer;

    timer_create(CLOCK_MONOTONIC,&sigev,&timer);
    timer_settime(timer,0,&its,NULL);

    // -----------

    sigset_t mis_seniales;
    sigemptyset(&mis_seniales);
    sigaddset(&mis_seniales,SIG_CINTA_B);

    // -----------

    int info;
    while(1){
        sigwait(&mis_seniales,&info);
        if(info == SIG_CINTA_B){
            printf("Cinta B: avisando a control de entrada de que tengo una nueva maleta\n");
            sigval_t val;
            sigqueue(getpid(),SIG_CINTA_B_NUEVA_MALETA,val);
        }
    }

}

void *control_entrada(void* arg) {
    
    struct itimerspec its;
    its.it_interval.tv_sec = 4;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1;

    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIG_ENTRADA;

    timer_t timer;

    timer_create(CLOCK_MONOTONIC,&sigev,&timer);
    timer_settime(timer,0,&its,NULL);

    // -----------

    sigset_t mis_seniales;
    sigemptyset(&mis_seniales);
    sigaddset(&mis_seniales,SIG_ENTRADA);
    sigaddset(&mis_seniales,SIG_CINTA_A_NUEVA_MALETA);
    sigaddset(&mis_seniales,SIG_CINTA_B_NUEVA_MALETA);

    // -----------

    int info;
    int tipo = 0; //0 para A, 1 para B;
    while(1){
        sigwait(&mis_seniales,&info);
        if(info == SIG_ENTRADA){
            printf("Control de entrada: Avisando al almacen para almacenar nuevas maletas.\n");
            sigval_t val;
            if(tipo == 0){//Enviar maletas de A
                sigqueue(getpid(),SIG_ENTRADA_CONTROL_A,val);
            }else{//Enviar maletas de B
                sigqueue(getpid(),SIG_ENTRADA_CONTROL_A,val);
            } 
        }else if (info == SIG_CINTA_A_NUEVA_MALETA) {
            printf("Control de entrada: Llegada de una maleta de la cinta A\n");
            tipo = 0;
        }else if (info == SIG_CINTA_B_NUEVA_MALETA) {
            printf("Control de entrada: Llegada de una maleta de la cinta B\n");
            tipo = 1;
        }
    }

}

void *controlAlmacenaje(void *arg){
    
    struct Almacen *almacen = (struct Almacen*)arg;
    
    // ---------------
        sigset_t mis_seniales;
        sigemptyset(&mis_seniales);
        sigaddset(&mis_seniales,SIG_ENTRADA_CONTROL_A);
        sigaddset(&mis_seniales,SIG_ENTRADA_CONTROL_B);
    // ---------------

    int info;
    while(1){
        sigwait(&mis_seniales, &info);
        if(info == SIG_ENTRADA_CONTROL_A) {
            pthread_mutex_lock(&almacen->mutex);
            almacen->maletasA++;
            printf("Control de almacenaje: almacenando una maleta de tipo A, maletas A: %d, maletas B: %d\n", almacen->maletasA, almacen->maletasB) ;
            pthread_mutex_unlock(&almacen->mutex);

        } else if(info == SIG_ENTRADA_CONTROL_B) {
            pthread_mutex_lock(&almacen->mutex);
            almacen->maletasB++;
            printf("Control de almacenaje: almacenando una maleta de tipo B, maletas A: %d, maletas B: %d\n", almacen->maletasA, almacen->maletasB) ;
            pthread_mutex_unlock(&almacen->mutex);
        }
    }

}

int main(){

    struct Almacen almacen;

    srand(time(NULL));
    mlockall(MCL_CURRENT | MCL_FUTURE);

    struct sched_param schedparam;
    schedparam.sched_priority = 50;
    pthread_setschedparam(pthread_self(),POLITICA,&schedparam);

    // -------------

    sigset_t todas_las_seniales;

    sigemptyset(&todas_las_seniales);
    sigaddset(&todas_las_seniales, SIG_ENTRADA_CONTROL_A);
    sigaddset(&todas_las_seniales, SIG_ENTRADA_CONTROL_B);
    sigaddset(&todas_las_seniales, SIG_ENTRADA);
    sigaddset(&todas_las_seniales, SIG_CINTA_B_NUEVA_MALETA);
    sigaddset(&todas_las_seniales, SIG_CINTA_A_NUEVA_MALETA);
    sigaddset(&todas_las_seniales, SIG_CINTA_A);
    sigaddset(&todas_las_seniales, SIG_CINTA_B);
    pthread_sigmask(SIG_BLOCK,&todas_las_seniales, NULL);


    // -------------
    
    pthread_t th_cintaA, th_cintaB, th_controlEntrada, th_controlAlmacenaje;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, POLITICA);
    pthread_attr_setschedparam(&attr,&schedparam);

    pthread_create(&th_cintaA, &attr, cintaA, NULL);
    pthread_create(&th_cintaB, &attr, cintaB, NULL);
    pthread_create(&th_controlEntrada, &attr, control_entrada, NULL);
    pthread_create(&th_controlAlmacenaje, &attr, controlAlmacenaje, &almacen);

    pthread_join(th_cintaA,NULL);
    pthread_join(th_cintaB,NULL);
    pthread_join(th_controlEntrada,NULL);
    pthread_join(th_controlAlmacenaje,NULL);

    return 0;

}