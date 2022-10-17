#include <stdio.h>
#include "users.h"
#include "items.h"
#include "protocol.h"

#define SEND_EMPTY_MSG(FD, TYPE, HEADER)({HEADER.msg_type = TYPE; HEADER.msg_len = 0; wr_msg(FD, &HEADER, NULL);})
#define SEND_S_MSG(FD, TYPE, HEADER, BUF, LEN)({HEADER.msg_type = TYPE; HEADER.msg_len = LEN; wr_msg(FD, &HEADER, BUF);})
#define SEND_FS_MSG(FD, TYPE, HEADER, FORMAT, ...)({\
                int r = snprintf(NULL, 0, FORMAT, __VA_ARGS__) + 1;\
                char* print_buf = malloc(r);\
                snprintf(print_buf, r, FORMAT, __VA_ARGS__);\
                HEADER.msg_type = TYPE; HEADER.msg_len = r;\
                wr_msg(FD, &HEADER, print_buf);\
                free(print_buf);\
        })

void logout(user_list_t* user_list, item_list_t* auction, char* client_name, int client_fd, int signal);
// USER_LIST: Writer, AUCTION: Writer
char* create_user_sales_str(user_list_t* user_list, item_list_t* auction, user_t* user, int* allocated_len);
// USER_LIST: Reader, AUCTION: Reader
char* create_user_wins_str(user_list_t* user_list, item_list_t* auction, user_t* user, int* allocated_len);
// USER_LIST: Reader, AUCTION: Reader
void process_bid(item_list_t* auction, user_list_t* user_list, petr_header h, user_t* client_user, int item_id, long bid_price);
// USER_LIST: Writer, AUCTION: Writer
void process_close(item_list_t* auction, user_list_t* user_list, petr_header h, user_t* client_user, int item_id);
// USER_LIST: Writer, AUCTION: Writer

// NON-CONCURRENCY-PROBLEMATIC FUNCTION
void file_to_auction(item_list_t* auction, char* file_name, int* auction_id);
int parse_msg(char* msg, int msg_len, char** buf_1, char** buf_2, char** buf_3);