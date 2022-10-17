#include "items.h"

item_list_t* create_item_list(pthread_mutex_t* lck, int* cnt, pthread_mutex_t* cnt_lck){
    item_list_t* item_list = malloc(sizeof(item_list_t));
    item_list->head = NULL;
    item_list->rdcnt = cnt;
    item_list->rdcnt_lck = cnt_lck;
    item_list->lck = lck;
    return item_list;
}

item_t* create_item(char* usr_n, int id, char* item_n, long bin_price, int duration){
    item_t* item = malloc(sizeof(item_t));
    item->id = id;
    item->host = usr_n;
    item->item_n = strdup(item_n);
    item->bin_price = bin_price;
    item->duration = duration;
    item->bid_price = 0;
    item->bid_usr_n = NULL;
    item->watch = NULL;
    item->num_watch = 0;
    item->next = NULL;
    return item;
}

void insert_item(item_list_t* item_list, item_t* item){
    pthread_mutex_lock(item_list->lck);
    if ( item_list->head == NULL ) {
        item_list->head = item;
    } else {
        item_t* current_item = item_list->head;
        while ( current_item->next != NULL ) {
            current_item = current_item->next;
        }
        current_item->next = item;
    }
    pthread_mutex_unlock(item_list->lck);
}

item_t* find_item_by_id(item_list_t* item_list, int id){
    item_t* current_item = NULL;
    pthread_mutex_lock(item_list->rdcnt_lck);
    *item_list->rdcnt++;
    if ( *item_list->rdcnt == 1 ) {
        pthread_mutex_lock(item_list->lck);
    }
    pthread_mutex_unlock(item_list->rdcnt_lck);

    if ( item_list->head != NULL ) {
        current_item = item_list->head;
        while ( current_item != NULL ) {
            if ( current_item->id == id ) {
                break;
            }
            current_item = current_item->next;
        }
    }

    pthread_mutex_lock(item_list->rdcnt_lck);
    *item_list->rdcnt--;
    if ( *item_list->rdcnt == 0 ) {
        pthread_mutex_unlock(item_list->lck);
    }
    pthread_mutex_unlock(item_list->rdcnt_lck);

    return current_item;
}

user_ref_t* create_user_ref(char* usr_n){
    user_ref_t* user_ref = malloc(sizeof(user_ref_t));
    user_ref->usr_n = usr_n;
    user_ref->next = NULL;
    return user_ref;
}

void insert_watch(item_list_t* item_list, item_t* item, char* usr_n){
    user_ref_t* user_ref = create_user_ref(usr_n);
    pthread_mutex_lock(item_list->lck);
    if ( item->watch == NULL ) {
        item->watch = user_ref;
        item->num_watch++;
    } else {
        user_ref->next = item->watch;
        item->watch = user_ref;
        item->num_watch++;
    }
    pthread_mutex_unlock(item_list->lck);
}

void remove_watch(item_list_t* item_list, item_t* item, char* usr_n){
    pthread_mutex_lock(item_list->lck);
    user_ref_t* prev_user = NULL;
    user_ref_t* current_user = item->watch;
    while ( current_user != NULL ) {
        if ( !strcmp(current_user->usr_n, usr_n) ) {
            if ( prev_user ) {
                prev_user->next = current_user->next;
            } else {
                item->watch = current_user->next;
            }
            free(current_user);
            item->num_watch--;
            break;
        }
        prev_user = current_user;
        current_user = current_user->next;
    }
    pthread_mutex_unlock(item_list->lck);
}

char* create_auction_str(item_list_t* auction, int* allocated_len){
    pthread_mutex_lock(auction->rdcnt_lck);
    *auction->rdcnt++;
    if ( *auction->rdcnt == 1 ) {
        pthread_mutex_lock(auction->lck);
    }
    pthread_mutex_unlock(auction->rdcnt_lck);

    char* buf;
    int r_len = 1;
    item_t* current_item = auction->head;
    while ( current_item != NULL ) {
        if ( current_item->duration > 0 ) {
            r_len += snprintf(NULL, 0, "%d;%s;%ld;%ld;%d;%d\n",
            current_item->id,
            current_item->item_n,
            current_item->bin_price,
            current_item->bid_price,
            current_item->num_watch,
            current_item->duration);
        }
        current_item = current_item->next;
    }
    if ( r_len == 1 ) { buf = NULL; }
    else {
        buf = malloc(r_len);
        int alloc_len = 0;
        current_item = auction->head;
        while ( current_item != NULL ) {
            if ( current_item->duration > 0 ) {
                alloc_len += snprintf(buf + alloc_len, r_len - alloc_len, "%d;%s;%ld;%ld;%d;%d\n",
                current_item->id,
                current_item->item_n,
                current_item->bin_price,
                current_item->bid_price,
                current_item->num_watch,
                current_item->duration);
            }
            current_item = current_item->next;
        }
        *allocated_len = alloc_len + 1;
    }
    pthread_mutex_lock(auction->rdcnt_lck);
    *auction->rdcnt--;
    if ( *auction->rdcnt == 0 ) {
        pthread_mutex_unlock(auction->lck);
    }
    pthread_mutex_unlock(auction->rdcnt_lck);
    return buf;
}
