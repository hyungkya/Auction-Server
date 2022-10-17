#ifndef ITEMS_H
#define ITEMS_H

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

typedef struct user_ref {
    char* usr_n;
    struct user_ref* next;
} user_ref_t;

typedef struct item {
    int id;
    char* host;
    char* item_n;
    long bin_price;
    int duration;
    long bid_price;
    char* bid_usr_n;
    user_ref_t* watch;
    int num_watch;
    struct item* next;
} item_t;

typedef struct item_list {
    item_t* head;
    int* rdcnt;
    pthread_mutex_t* rdcnt_lck;
    pthread_mutex_t* lck;
} item_list_t;

item_list_t* create_item_list(pthread_mutex_t* lck, int* cnt, pthread_mutex_t* cnt_lck); // None
item_t* create_item(char* usr_n, int id, char* item_n, long bin_price, int duration); // None
void insert_item(item_list_t* item_list, item_t* item); // Writer
item_t* find_item_by_id(item_list_t* item_list, int id); // Reader
user_ref_t* create_user_ref(char* usr_n); // None
void insert_watch(item_list_t* item_list, item_t* item, char* usr_n); // Writer
void remove_watch(item_list_t* item_list, item_t* item, char* usr_n); // Writer
char* create_auction_str(item_list_t* auction, int* allocated_len); // Reader

#endif