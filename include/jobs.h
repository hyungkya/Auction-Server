#ifndef JOBS_H
#define JOBS_H

#include <stdlib.h>
#include <string.h>
#include "protocol.h"
#include <semaphore.h>
#include <pthread.h>

typedef struct job {
    char* usr_n;
    int fd;
    petr_header msg_h;
    char* msg;
    struct job* next;
} job_t;

typedef struct job_list {
    job_t* head;
    sem_t* item_s;
    sem_t* slot_s;
    pthread_mutex_t* lck;
} job_list_t;

job_list_t* create_job_list(pthread_mutex_t* lck, sem_t* slot_s, sem_t* item_s);

job_t* create_job(char* usr_n, int fd, petr_header msg_h, char* msg);
void delete_job(job_t* job);

void put_job(job_list_t* job_list, job_t* job);
job_t* get_job(job_list_t* job_list);

#endif