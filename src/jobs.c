#include "jobs.h"
#include <stdio.h>

job_list_t* create_job_list(pthread_mutex_t* lck, sem_t* slot_s, sem_t* item_s) {
    job_list_t* job_list = malloc(sizeof(job_list_t));
    job_list->head = NULL;
    job_list->lck = lck;
    job_list->slot_s = slot_s;
    job_list->item_s = item_s;

    // Initiating given semaphores
    sem_init(slot_s, 0, 64);
    sem_init(item_s, 0, 0);
    
    return job_list;
}

job_t* create_job(char* usr_n, int fd, petr_header msg_h, char* msg){
    job_t* job = malloc(sizeof(job_t));
    job->usr_n = usr_n;
    job->fd = fd;
    job->msg_h = msg_h;
    job->msg = strdup(msg);
    job->next = NULL;
    return job;
}

void delete_job(job_t* job){
    free(job->msg);
    free(job);
}

void put_job(job_list_t* job_list, job_t* job){    
    sem_wait(job_list->slot_s);
    pthread_mutex_lock(job_list->lck);

    if ( job_list->head == NULL ) {
        job_list->head = job;
    } else {
        job_t* current_job = job_list->head;
        while ( current_job->next != NULL ) {
            current_job = current_job->next;
        }
        current_job->next = job;
    }
    pthread_mutex_unlock(job_list->lck);
    sem_post(job_list->item_s);
}

job_t* get_job(job_list_t* job_list){    
    sem_wait(job_list->item_s);
    pthread_mutex_lock(job_list->lck);

    job_t* job;
    if ( job_list->head == NULL ) { 
        job = NULL; 
    }
    else {
        job = job_list->head;
        job_list->head = job_list->head->next;
    }

    pthread_mutex_unlock(job_list->lck);
    sem_post(job_list->slot_s);
    
    return job;
}