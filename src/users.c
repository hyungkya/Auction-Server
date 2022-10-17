#include "users.h"

user_list_t* create_user_list(pthread_mutex_t* lck, int* cnt, pthread_mutex_t* cnt_lck){
    user_list_t* user_list = malloc(sizeof(user_list_t));
    user_list->head = NULL;
    user_list->rdcnt = cnt;
    user_list->rdcnt_lck = cnt_lck;
    user_list->lck = lck;
    return user_list;
}

user_t* create_user(char* usr_n, char* pwd, int fd){
    user_t* user = malloc(sizeof(user_t));
    user->usr_n = strdup(usr_n);
    user->pwd = strdup(pwd);
    user->fd = fd;
    user->watch = NULL;
    user->host = NULL;
    user->win = NULL;
    user->balance = 0;
    user->next = NULL;
    return user;
}

item_ref_t* create_item_ref(int id){
    item_ref_t* item_ref = malloc(sizeof(item_ref_t));
    item_ref->item_id = id;
    item_ref->next = NULL;
    return item_ref;
}

void insert_user(user_list_t* user_list, user_t* user){
    pthread_mutex_lock(user_list->lck);
    if ( user_list->head == NULL ) {
        user_list->head = user;
    } else {
        user->next = user_list->head;
        user_list->head = user;
    }
    pthread_mutex_unlock(user_list->lck);
}

user_t* find_user_by_name(user_list_t* user_list, char* usr_n){
    pthread_mutex_lock(user_list->rdcnt_lck);
    *user_list->rdcnt++;
    if ( *user_list->rdcnt == 1 ) {
        pthread_mutex_lock(user_list->lck);
    }
    pthread_mutex_unlock(user_list->rdcnt_lck);

    user_t* user = NULL;
    if ( user_list->head != NULL ) {
        user_t* current_user = user_list->head;
        while ( current_user != NULL ) {
            if ( !strcmp(current_user->usr_n, usr_n) ) {
                user = current_user;
            }
            current_user = current_user->next;
        }
    }
    
    pthread_mutex_lock(user_list->rdcnt_lck);
    *user_list->rdcnt--;
    if ( *user_list->rdcnt == 0 ) {
        pthread_mutex_unlock(user_list->lck);
    }
    pthread_mutex_unlock(user_list->rdcnt_lck);

    return user;
}

user_t* find_user_by_fd(user_list_t* user_list, int fd){
    pthread_mutex_lock(user_list->rdcnt_lck);
    *user_list->rdcnt++;
    if ( *user_list->rdcnt == 1 ) {
        pthread_mutex_lock(user_list->lck);
    }
    pthread_mutex_unlock(user_list->rdcnt_lck);

    user_t* user = NULL;
    if ( user_list->head != NULL ) {
        user_t* current_user = user_list->head;
        while ( current_user != NULL ) {
            if ( current_user->fd == fd ) {
                user = current_user;
            }
            current_user = current_user->next;
        }
    }

    pthread_mutex_lock(user_list->rdcnt_lck);
    *user_list->rdcnt--;
    if ( *user_list->rdcnt == 0 ) {
        pthread_mutex_unlock(user_list->lck);
    }
    pthread_mutex_unlock(user_list->rdcnt_lck);
    
    return user;
}

void insert_item_ref(user_list_t* user_list, user_t* user, int idx, item_ref_t* item_ref){
    // WATCH = 0, HOST = 1, WIN = 2
    pthread_mutex_lock(user_list->lck);
    if ( idx == 0 ) {
        if ( user->watch == NULL ) {
            user->watch = item_ref;
        } else {
            item_ref->next = user->watch;
            user->watch = item_ref;
        }
    }
    if ( idx == 1 ) {
        if ( user->host == NULL ) {
            user->host = item_ref;
        } else {
            item_ref->next = user->host;
            user->host = item_ref;
        }
    }
    if ( idx == 2 ) {
        if ( user->win == NULL ) {
            user->win = item_ref;
        } else {
            item_ref->next = user->win;
            user->win = item_ref;
        }
    }
    pthread_mutex_unlock(user_list->lck);
}

int remove_item_ref(user_list_t* user_list, user_t* user, int id){
    // REMOVED = 1, NOT REMOVED = 0
    int removal_signal = 0;
    pthread_mutex_lock(user_list->lck);
    if ( user->watch != NULL ) {
        item_ref_t* prev_ref = NULL;
        item_ref_t* current_ref = user->watch;
        while ( current_ref != NULL ) {
            if ( current_ref->item_id == id ) {
                if ( prev_ref ) {
                    prev_ref->next = current_ref->next;
                } else {
                    user->watch = NULL;
                }
                free(current_ref);
                removal_signal = 1;
                break;
            }
            prev_ref = current_ref;
            current_ref = current_ref->next;
        }
    }
    pthread_mutex_unlock(user_list->lck);
    return removal_signal;
}


char* create_user_list_str(user_list_t* user_list, char* usr_n, int* allocated_len){
    pthread_mutex_lock(user_list->rdcnt_lck);
    *user_list->rdcnt++;
    if ( *user_list->rdcnt == 1 ) {
        pthread_mutex_lock(user_list->lck);
    }
    pthread_mutex_unlock(user_list->rdcnt_lck);

    char* buf;
    int r_len = 1;
    user_t* current_user = user_list->head;
    while ( current_user != NULL ) {
        if ( current_user->fd != -1 && strcmp(current_user->usr_n, usr_n) ) {
            r_len += snprintf(NULL, 0, "%s\n", current_user->usr_n);
        }
        current_user = current_user->next;
    }
    if ( r_len == 1 ) { buf = NULL; }
    else { 
        int alloc_len = 0;
        buf = malloc(r_len);
        current_user = user_list->head;
        while ( current_user != NULL ) {
            if ( current_user->fd != -1 && strcmp(current_user->usr_n, usr_n) ) {
                alloc_len += snprintf(buf + alloc_len, r_len - alloc_len, "%s\n", current_user->usr_n);
            }
            current_user = current_user->next;
        }
        *allocated_len = alloc_len + 1;
    }
    
    pthread_mutex_lock(user_list->rdcnt_lck);
    *user_list->rdcnt--;
    if ( *user_list->rdcnt == 0 ) {
        pthread_mutex_unlock(user_list->lck);
    }
    pthread_mutex_unlock(user_list->rdcnt_lck);

    return buf;
}