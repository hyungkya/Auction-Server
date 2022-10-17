#include "auction_util.h"

void logout(user_list_t* user_list, item_list_t* auction, char* client_name, int client_fd, int signal){
    pthread_mutex_lock(auction->lck);
    pthread_mutex_lock(user_list->lck);
    if ( user_list->head != NULL ) {
        user_t* current_user = find_user_by_name(user_list, client_name);
        if ( current_user != NULL ) {
            if ( current_user->fd == client_fd ) {
                while ( current_user->watch != NULL ) {
                    item_t* item = find_item_by_id(auction, current_user->watch->item_id);
                    remove_watch(auction, item, current_user->usr_n);
                    item_ref_t* temp = current_user->watch;
                    current_user->watch = current_user->watch->next;
                    free(temp);
                }
                current_user->fd = -1;
                if ( signal ) {
                    petr_header h = {0};
                    h.msg_type = OK;
                    h.msg_len = 0;
                    wr_msg(client_fd, &h, NULL);
                }
            }
        }
    }
    pthread_mutex_unlock(user_list->lck);
    pthread_mutex_unlock(auction->lck);
}

char* create_user_sales_str(user_list_t* user_list, item_list_t* auction, user_t* user, int* allocated_len){
    pthread_mutex_lock(auction->rdcnt_lck);
    *auction->rdcnt++;
    if ( *auction->rdcnt == 1 ) {
        pthread_mutex_lock(auction->lck);
    }
    pthread_mutex_unlock(auction->rdcnt_lck);

    pthread_mutex_lock(user_list->rdcnt_lck);
    *user_list->rdcnt++;
    if ( *user_list->rdcnt == 1 ) {
        pthread_mutex_lock(user_list->lck);
    }
    pthread_mutex_unlock(user_list->rdcnt_lck);

    char* buf;
    int r_len = 1;
    item_ref_t* item_ref = user->host;
    while ( item_ref != NULL ) {
        item_t* item = find_item_by_id(auction, item_ref->item_id);
        r_len += snprintf(NULL, 0, "%d;%s;%s;%ld\n", item->id, item->item_n, item->bid_usr_n ? item->bid_usr_n : "None", item->bid_price);
        item_ref = item_ref->next;
    }
    if ( r_len == 1 ) {
        buf = NULL;
    } else {
        buf = malloc(r_len);
        int alloc_len = 0;
        item_ref = user->host;
        while ( item_ref != NULL ) {
            item_t* item = find_item_by_id(auction, item_ref->item_id);
            alloc_len += snprintf(buf + alloc_len, r_len - alloc_len, "%d;%s;%s;%ld\n", item->id, item->item_n, item->bid_usr_n ? item->bid_usr_n : "None", item->bid_price);
            item_ref = item_ref->next;
        }
        *allocated_len = alloc_len + 1;
    }

    pthread_mutex_lock(user_list->rdcnt_lck);
    *user_list->rdcnt--;
    if ( *user_list->rdcnt == 0 ) {
        pthread_mutex_unlock(user_list->lck);
    }
    pthread_mutex_unlock(user_list->rdcnt_lck);

    pthread_mutex_lock(auction->rdcnt_lck);
    *auction->rdcnt--;
    if ( *auction->rdcnt == 0 ) {
        pthread_mutex_unlock(auction->lck);
    }
    pthread_mutex_unlock(auction->rdcnt_lck);

    return buf;
}

char* create_user_wins_str(user_list_t* user_list, item_list_t* auction, user_t* user, int* allocated_len){
    pthread_mutex_lock(auction->rdcnt_lck);
    *auction->rdcnt++;
    if ( *auction->rdcnt == 1 ) {
        pthread_mutex_lock(auction->lck);
    }
    pthread_mutex_unlock(auction->rdcnt_lck);

    pthread_mutex_lock(user_list->rdcnt_lck);
    *user_list->rdcnt++;
    if ( *user_list->rdcnt == 1 ) {
        pthread_mutex_lock(user_list->lck);
    }
    pthread_mutex_unlock(user_list->rdcnt_lck);
    char* buf;
    int r_len = 1;
    item_ref_t* item_ref = user->win;
    while ( item_ref != NULL ) {
        item_t* item = find_item_by_id(auction, item_ref->item_id);
        r_len += snprintf(NULL, 0, "%d;%s;%ld\n", item->id, item->item_n, item->bid_price);
        item_ref = item_ref->next;
    }
    if ( r_len == 1 ) {
        buf = NULL;
    } else {
        buf = malloc(r_len);
        int alloc_len = 0;
        item_ref = user->win;
        while ( item_ref != NULL ) {
            item_t* item = find_item_by_id(auction, item_ref->item_id);
            alloc_len += snprintf(buf + alloc_len, r_len - alloc_len, "%d;%s;%ld\n", item->id, item->item_n, item->bid_price);
            item_ref = item_ref->next;
        }
        *allocated_len = alloc_len + 1;
    }

    pthread_mutex_lock(user_list->rdcnt_lck);
    *user_list->rdcnt--;
    if ( *user_list->rdcnt == 0 ) {
        pthread_mutex_unlock(user_list->lck);
    }
    pthread_mutex_unlock(user_list->rdcnt_lck);

    pthread_mutex_lock(auction->rdcnt_lck);
    *auction->rdcnt--;
    if ( *auction->rdcnt == 0 ) {
        pthread_mutex_unlock(auction->lck);
    }
    pthread_mutex_unlock(auction->rdcnt_lck);

    return buf;
}

void file_to_auction(item_list_t* auction, char* file_name, int* auction_id){
    if ( file_name == NULL ) {
        return;
    }
    char* buf = malloc(256);
    FILE* fd = fopen(file_name, "r");
    buf = fgets(buf, 256, fd); // FIRST LINE
    while (1) {
        if ( buf == NULL ) { break; } // IF FIRST LINE is NULL ... BREAK !
        char* item_name = strndup(buf, strcspn(buf, "\n"));
        bzero(buf, 256);
        buf = fgets(buf, 256, fd); // SECOND LINE !
        int duration = atoi(buf);
        bzero(buf, 256);
        buf = fgets(buf, 256, fd); // THIRD LINE !
        long price = atol(buf);
        bzero(buf, 256);
        buf = fgets(buf, 256, fd); // FOURTH LINE ! ( \n )
        buf = fgets(buf, 256, fd); // FIFTH LINE ! ( = FIRST LINE )
        insert_item(auction, create_item(NULL, (*auction_id)++, item_name, price, duration));
    }
    free(buf);
}

void process_bid(item_list_t* auction, user_list_t* user_list, petr_header h, user_t* client_user, int item_id, long bid_price){
    // SERIES OF VALIDITY CHECKS
    item_t* target_item;
    if ( (target_item = find_item_by_id(auction, item_id)) == NULL ) { SEND_EMPTY_MSG(client_user->fd, EANNOTFOUND, h); }
    else {
        user_ref_t* watcher_ref = target_item->watch;
        int is_watching = 0;
        while ( watcher_ref != NULL ) {
            if ( !strcmp(watcher_ref->usr_n, client_user->usr_n) ) {
                is_watching = 1;
            }
            watcher_ref = watcher_ref->next;
        }
        if ( !is_watching ) { SEND_EMPTY_MSG(client_user->fd, EANDENIED, h); }
        else if ( !strcmp(target_item->host, client_user->usr_n) ) { SEND_EMPTY_MSG(client_user->fd, EANDENIED, h); } 
        else if ( target_item->bid_price >= bid_price ) { SEND_EMPTY_MSG(client_user->fd, EBIDLOW, h); }
        else {
            // VALID ! UPDATE BID
            target_item->bid_usr_n = client_user->usr_n;
            target_item->bid_price = bid_price;

            if ( target_item->bin_price != 0 && bid_price >= target_item->bin_price ) {
                target_item->duration = 0;
                process_close(auction, user_list, h, client_user, item_id);
            } 
            else {
                // SEND "ANUPDATE" TO WATCHERS
                watcher_ref = target_item->watch;
                while ( watcher_ref != NULL ) {
                    user_t* watcher = find_user_by_name(user_list, watcher_ref->usr_n);
                    if ( watcher->fd != -1 ) {
                        SEND_FS_MSG(watcher->fd, ANUPDATE, h, "%d\r\n%s\r\n%s\r\n%ld", target_item->id, target_item->item_n, client_user->usr_n, bid_price);
                    }
                    watcher_ref = watcher_ref->next;
                }
            }
            // SEND "OK" TO BIDER
            SEND_EMPTY_MSG(client_user->fd, OK, h);
        }
    }
}

void process_close(item_list_t* auction, user_list_t* user_list, petr_header h, user_t* client_user, int item_id){
    char* buf;
    int r_len;
    
    item_t* item = find_item_by_id(auction, item_id);
    if ( item->bid_usr_n != NULL ) {
        // ADD WIN TO WINNER
        user_t* winner = find_user_by_name(user_list, item->bid_usr_n);
        insert_item_ref(user_list, winner, 2, create_item_ref(item->id));
        winner->balance -= item->bid_price;

        // CREATING MESSAGE
        r_len = snprintf(NULL, 0, "%d\r\n%s\r\n%ld", item->id, item->bid_usr_n, item->bid_price) + 1;
        buf = malloc(r_len);
        snprintf(buf, r_len, "%d\r\n%s\r\n%ld", item->id, item->bid_usr_n, item->bid_price);
    } else {
        // CREATING MESSAGE
        r_len = snprintf(NULL, 0, "%d\r\n\r\n", item->id) + 1;
        buf = malloc(r_len);
        snprintf(buf, r_len, "%d\r\n\r\n", item->id);
    }

    // ADD SALE TO HOST
    user_t* host = find_user_by_name(user_list, item->host);
    insert_item_ref(user_list, host, 1, create_item_ref(item->id));
    host->balance += item->bid_price;

    // SEND MESSAGE TO WATCHER
    user_ref_t* watcher_ref = item->watch;
    while ( watcher_ref != NULL ) {
        user_t* watcher = find_user_by_name(user_list, watcher_ref->usr_n);
        if ( watcher->fd != -1 ) {
            SEND_S_MSG(watcher->fd, ANCLOSED, h, buf, r_len);
        }
        watcher_ref = watcher_ref->next;
    }
}

int parse_msg(char* msg, int msg_len, char** buf_1, char** buf_2, char** buf_3){
    int max_c = buf_1 ? (buf_2 ? (buf_3 ? 3 : 2) : 1) : 0;
    int use_c = 0;
    char prev_chr = 0;
    for ( int i = 0, j = 0; i < msg_len && use_c < max_c; i++ ) {
        if ( msg[i] == 0 || msg[i] == 10 || msg[i] == 13 ) {
            if ( prev_chr != 0 ) {
                if ( use_c == 0 ) { *buf_1 = msg + j; j = i + 1; use_c++; }
                else if ( use_c == 1 ) { *buf_2 = msg + j; j = i + 1; use_c++; }
                else if ( use_c == 2 ) { *buf_3 = msg + j; use_c++; }
            } else {
                j++;
            }
            msg[i] = 0;
        }
        prev_chr = msg[i];
    }
    return use_c;
}