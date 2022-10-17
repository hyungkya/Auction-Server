#ifndef USERS_H
#define USERS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

typedef struct item_ref {
    int item_id;
    struct item_ref* next;
} item_ref_t;

typedef struct user {
    char* usr_n;
    char* pwd;
    int fd;
    item_ref_t* watch;
    item_ref_t* host;
    item_ref_t* win;
    long balance;
    struct user* next;
} user_t;

typedef struct user_list {
    user_t* head;
    int* rdcnt;
    pthread_mutex_t* rdcnt_lck;
    pthread_mutex_t* lck;
} user_list_t;

user_list_t* create_user_list(pthread_mutex_t* lck, int* cnt, pthread_mutex_t* cnt_lck);
user_t* create_user(char* usr_n, char* pwd, int fd); // None
item_ref_t* create_item_ref(int id); // None
void insert_user(user_list_t* user_list, user_t* user); // Writer
user_t* find_user_by_name(user_list_t* user_list, char* usr_n); // Reader
user_t* find_user_by_fd(user_list_t* user_list, int fd); // Reader
void insert_item_ref(user_list_t* user_list, user_t* user, int idx, item_ref_t* trg); // Writer
int remove_item_ref(user_list_t* user_list, user_t* user, int id); // Writer
char* create_user_list_str(user_list_t* user_list, char* usr_n, int* allocated_len); // Reader

#endif