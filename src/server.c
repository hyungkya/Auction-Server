// ICS 53: FINAL PROJECT
// GROUP 13
// Hyungkyu An (hyungkya)
// Seungmin You (seungmy)

#include "server.h"
#include "users.h"
#include "items.h"
#include "jobs.h"
#include "auction_util.h"
#include "protocol.h"

#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

const char exit_str[] = "exit";

char buffer[BUFFER_SIZE];
pthread_mutex_t buffer_lock; 

user_list_t* user_list;
pthread_mutex_t u_lck;
int u_rdcnt = 0;
pthread_mutex_t u_rdcnt_lck;

item_list_t* auction;
pthread_mutex_t a_lck;
int a_rdcnt = 0;
pthread_mutex_t a_rdcnt_lck;

int auc_id;
pthread_mutex_t id_lck;

job_list_t* job_list;
sem_t item_s;
sem_t slot_s;
pthread_mutex_t j_lck;

int listen_fd;

void sigint_handler(int sig)
{
    close(listen_fd);
    exit(0);
}

int server_init(int server_port){
    int sockfd;
    struct sockaddr_in servaddr;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully created\n");

    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(server_port);

    int opt = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt))<0)
    {
	perror("setsockopt");exit(EXIT_FAILURE);
    }

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully binded\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 1)) != 0) {
        printf("Listen failed\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Server listening on port: %d.. Waiting for connection\n", server_port);

    return sockfd;
}

void *client_thread(void* clientfd_ptr) {
    /*
        Input: client_fd integer pointer as void pointer form
        Function: continuously listens to the corresponding file descriptor and queue that message into job thread.
                  if 1) rg_msgheader fails / 2) LOGOUT signal catched, client thread closes.
    */
    int client_fd = *(int *)clientfd_ptr;
    char* client_name = find_user_by_fd(user_list, client_fd)->usr_n;
    free(clientfd_ptr);

    petr_header h = {0};
    fd_set read_fds;
    int retval;
    int logout_signal = 0;
    
    while(1){
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        retval = select(client_fd + 1, &read_fds, NULL, NULL, NULL);
        if (retval!=1 && !FD_ISSET(client_fd, &read_fds)){
            printf("SELECT() FAILED\n");
            break;
        }
        
        if ( rd_msgheader(client_fd, &h) == 0 ) {  // if rd_msgheader returns success signal
            if ( h.msg_type == LOGOUT ) { logout_signal = 1; break; } // close case #1

            pthread_mutex_lock(&buffer_lock);
            bzero(buffer, BUFFER_SIZE);

            read(client_fd, buffer, h.msg_len);
            job_t* job = create_job(client_name, client_fd, h, buffer);

            bzero(buffer, BUFFER_SIZE);
            pthread_mutex_unlock(&buffer_lock); 

            put_job(job_list, job);
        } 
        else { 
            break; 
        }
    }
    logout(user_list, auction, client_name, client_fd, logout_signal);
    close(client_fd);
    return NULL;
}

void* job_thread(void* vargp) {
    /*
        Function: Pop a job from job queue and execute corresponding functions.
    */
    pthread_detach(pthread_self());
    while(1) {
        job_t* job = get_job(job_list);

        petr_header h = {0};
        user_t* client_user = NULL;
        // -2 indicates server-generated messages that doesn't have usr_n in job.
        if ( job->fd != -2 ) { client_user = find_user_by_name(user_list, job->usr_n); }

        // If matching types exist, corresponding actions will be executed. If not, nothing happens.
        if ( job->msg_h.msg_type == ANCREATE ) {
            char* item_name = NULL;
            char* duration_str = NULL; 
            char* price_str = NULL;
            int parsed_argc = parse_msg(job->msg, job->msg_h.msg_len, &item_name, &duration_str, &price_str);
            if ( parsed_argc != 3 ) { SEND_EMPTY_MSG(job->fd, EINVALIDARG, h); }
            else {
                int duration = atoi(duration_str);
                long price = atol(price_str);

                if ( duration <= 0 || price < 0 ) {
                    SEND_EMPTY_MSG(job->fd, EINVALIDARG, h);
                } else {
                    pthread_mutex_lock(&id_lck);
                    int assigned_id = auc_id++;
                    pthread_mutex_unlock(&id_lck);
                    item_t* new_item = create_item(job->usr_n, assigned_id, item_name, price, duration);
                    insert_item(auction, new_item);
                    SEND_FS_MSG(job->fd, ANCREATE, h, "%d", assigned_id);
                }
            }
        } else if ( job->msg_h.msg_type == ANLIST ){
            int allocated_len = 0;
            char* auction_str = create_auction_str(auction, &allocated_len);
            if ( !auction_str ) { SEND_EMPTY_MSG(job->fd, ANLIST, h); }
            else {
                SEND_S_MSG(job->fd, ANLIST, h, auction_str, allocated_len);
                free(auction_str);
            }
        } else if ( job->msg_h.msg_type == ANWATCH ){
            char* id_str;
            int parsed_argc = parse_msg(job->msg, job->msg_h.msg_len, &id_str, NULL, NULL);
            if ( parsed_argc != 1 ) { SEND_EMPTY_MSG(job->fd, EINVALIDARG, h); }
            else {
                int item_id = atoi(job->msg);
                if ( item_id > 0 ) {
                    item_t* target_item = find_item_by_id(auction, item_id);
                    if ( target_item ) {
                        insert_watch(auction, target_item, job->usr_n);
                        insert_item_ref(user_list, client_user, 0, create_item_ref(item_id));
                        SEND_FS_MSG(job->fd, ANWATCH, h, "%s\r\n%ld", target_item->item_n, target_item->bin_price);
                    } else {
                        SEND_EMPTY_MSG(job->fd, EANNOTFOUND, h);
                    }
                } else {
                    SEND_EMPTY_MSG(job->fd, EINVALIDARG, h);
                }
            }
        } else if ( job->msg_h.msg_type == ANLEAVE ){
            char* id_str;
            int parsed_argc = parse_msg(job->msg, job->msg_h.msg_len, &id_str, NULL, NULL);
            if ( parsed_argc != 1 ) { SEND_EMPTY_MSG(job->fd, EINVALIDARG, h); }
            else {
                int item_id = atoi(id_str);
                if ( item_id <= 0 ) { SEND_EMPTY_MSG(job->fd, EINVALIDARG, h); }
                else {
                    item_t* target_item = find_item_by_id(auction, item_id);
                    if ( target_item ) {
                        remove_watch(auction, target_item, job->usr_n);
                        remove_item_ref(user_list, client_user, item_id);
                        SEND_EMPTY_MSG(job->fd, OK, h);
                    } else {
                        SEND_EMPTY_MSG(job->fd, EANNOTFOUND, h);
                    }
                }
            }
        } else if ( job->msg_h.msg_type == ANBID ){
            char* auction_id_str = NULL;
            char* bid_str = NULL;
            int parsed_argc = parse_msg(job->msg, job->msg_h.msg_len, &auction_id_str, &bid_str, NULL);
            if ( parsed_argc != 2 ) { SEND_EMPTY_MSG(job->fd, EINVALIDARG, h); }
            else {
                int item_id = atoi(auction_id_str);
                long bid_price = atol(bid_str);
                pthread_mutex_lock(&a_lck);
                pthread_mutex_lock(&u_lck);
                process_bid(auction, user_list, h, client_user, item_id, bid_price);
                pthread_mutex_unlock(&u_lck);
                pthread_mutex_unlock(&a_lck);
            } 
        } else if ( job->msg_h.msg_type == ANUPDATE ){
            char* id_str;
            int parsed_argc = parse_msg(job->msg, job->msg_h.msg_len, &id_str, NULL, NULL);
            if ( parsed_argc != 1 ) { SEND_EMPTY_MSG(job->fd, EINVALIDARG, h); }
            else {
                int item_id = atoi(id_str);
                pthread_mutex_lock(&a_lck);
                pthread_mutex_lock(&u_lck);
                process_close(auction, user_list, h, client_user, item_id);
                pthread_mutex_unlock(&u_lck);
                pthread_mutex_unlock(&a_lck);
            }
        } else if ( job->msg_h.msg_type == USRLIST ){
            // Concurrency-froof.
            int allocated_len = 0;
            char* user_list_str = create_user_list_str(user_list, job->usr_n, &allocated_len);
            if ( user_list_str ) {
                SEND_S_MSG(job->fd, USRLIST, h, user_list_str, allocated_len);
                free(user_list_str);
            } else {
                SEND_EMPTY_MSG(job->fd, USRLIST, h);
            }

        } else if ( job->msg_h.msg_type == USRWINS ){
            // Concurrency-froof.
            int allocated_len = 0;
            char* user_wins_str = create_user_wins_str(user_list, auction, client_user, &allocated_len);
            if ( user_wins_str ) {
                SEND_S_MSG(job->fd, USRWINS, h, user_wins_str, allocated_len);
            } else {
                SEND_EMPTY_MSG(job->fd, USRWINS, h);
            }

        } else if ( job->msg_h.msg_type == USRSALES ){
            // Concurrency-froof.
            int allocated_len = 0;
            char* user_sales_str = create_user_sales_str(user_list, auction, client_user, &allocated_len);
            if ( user_sales_str ) {
                SEND_S_MSG(job->fd, USRSALES, h, user_sales_str, allocated_len);
            } else {
                SEND_EMPTY_MSG(job->fd, USRSALES, h);
            }

        } else if ( job->msg_h.msg_type == USRBLNC ){
            // Concurrency-froof.
            SEND_FS_MSG(job->fd, USRBLNC, h, "%ld", client_user->balance);
        }
        delete_job(job);
    }
    return NULL;
}

void* tick_thread(void* vargp) {
    // Concurrency-froof.
    int seconds_per_tick = *(int*) vargp;
    free(vargp);
    pthread_detach(pthread_self());
    char buf[256];
    char* print_buf;

    while(1) {
        if ( seconds_per_tick > 0 ) { sleep(seconds_per_tick); }
        else { fgets(buf, 256, stdin); }

        pthread_mutex_lock(&a_lck);
        item_t* current_item = auction->head;
        while ( current_item != NULL ) {
            if ( current_item->duration > 0 ) {
                current_item->duration--;
                if ( current_item->duration == 0 ) {
                    int r_len = snprintf(NULL, 0, "%d", current_item->id) + 1;
                    print_buf = malloc(r_len);
                    snprintf(print_buf, r_len, "%d", current_item->id);

                    petr_header h = {0};
                    h.msg_type = ANUPDATE;
                    h.msg_len = r_len;

                    job_t* job = create_job(NULL, -2, h, print_buf);
                    put_job(job_list, job);

                    free(print_buf);
                }
            }
            current_item = current_item->next;
        }
        pthread_mutex_unlock(&a_lck);
    }    
    return NULL;
}

int verify_user(int client_fd) {
    // Concurrency-froof.
    petr_header h = {0};
    char* username_buf; char* password_buf;
    int username_len = 0, password_len = 0;

    if ( rd_msgheader(client_fd, &h) != 0 || h.msg_type != LOGIN ) { return -1; }
    
    pthread_mutex_lock(&buffer_lock);
    bzero(buffer, BUFFER_SIZE);

    read(client_fd, buffer, h.msg_len);
    int parsed_argc = parse_msg(buffer, h.msg_len, &username_buf, &password_buf, NULL);
    if ( parsed_argc == 2 ) {
        username_buf = strdup(username_buf);
        password_buf = strdup(password_buf);
    }
    bzero(buffer, BUFFER_SIZE);
    pthread_mutex_unlock(&buffer_lock);
    if ( parsed_argc != 2 ) { return - 1; } // SOMETHING WENT WRONG WITH PARSING OR MESSAGE

    user_t* user = find_user_by_name(user_list, username_buf);
    if ( !user ) {
        insert_user(user_list, create_user(username_buf, password_buf, client_fd));
        SEND_EMPTY_MSG(client_fd, OK, h);
    } else {
        // if fd == -1, it represents "logged out" status
        if ( user->fd != -1 ) { SEND_EMPTY_MSG(client_fd, EUSRLGDIN, h); return -1; }
        if ( strcmp(user->pwd, password_buf) ) { SEND_EMPTY_MSG(client_fd, EWRNGPWD, h); return -1; }
        user->fd = client_fd;
        SEND_EMPTY_MSG(client_fd, OK, h);
    }
    return 0;
}

void run_server(int server_port, int number_of_job_threads, int seconds_per_tick, char* auction_file){
    // Initiate Global Variables
    user_list = create_user_list(&u_lck, &u_rdcnt, &u_rdcnt_lck);
    auction = create_item_list(&a_lck, &a_rdcnt, &a_rdcnt_lck);
    job_list = create_job_list(&j_lck, &slot_s, &item_s);
    auc_id = 1;

    pthread_t tid;
    pthread_t d_tid[number_of_job_threads + 1]; //TODO ...

    // Initiate Tick-Thread
    int* tick_ptr = malloc(sizeof(int));
    *tick_ptr = seconds_per_tick;
    pthread_create(&d_tid[0], NULL, tick_thread, tick_ptr);

    // Initiate Job-Thread
    int num_job_thread = number_of_job_threads;
    for ( int i = 0; i < num_job_thread; i++ ) {
        pthread_create(&d_tid[i + 1], NULL, job_thread, NULL);
    }

    file_to_auction(auction, auction_file, &auc_id);

    listen_fd = server_init(server_port); // Initiate server and start listening on specified port
    int client_fd;
    struct sockaddr_in client_addr;
    unsigned int client_addr_len = sizeof(client_addr);
 
    while(1) {
        int* client_fd = malloc(sizeof(int));
        *client_fd = accept(listen_fd, (SA*)&client_addr, &client_addr_len);
        if (*client_fd < 0) { exit(EXIT_FAILURE); }

        if ( !verify_user(*client_fd) ) { pthread_create(&tid, NULL, client_thread, (void *)client_fd); } 
        else { free(client_fd); }
    }
    close(listen_fd);
    return;
}

int main(int argc, char* argv[])
{
    int opt = -1;
    unsigned int port = 0;
    int job_threads_num = 2;
    int seconds_per_tick = 0;
    char* auction_file_name = NULL;

    while ((opt = getopt(argc, argv, "h:j:t:")) != -1) {
        switch (opt) {
        case 'h':
            printf(USAGE_MSG);
            exit(EXIT_SUCCESS);
        case 'j':
            job_threads_num = atoi(optarg);
            break;
        case 't':
            seconds_per_tick = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Server Application Usage: %s -p <port_number>\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    port = atoi(argv[optind]);
    if (port == 0){
        fprintf(stderr, "ERROR: Port number for server to listen is not given\n");
        fprintf(stderr, "Server Application Usage: %s -p <port_number>\n",
                    argv[0]);
        exit(EXIT_FAILURE);
    }
    auction_file_name = argv[optind+1];
    run_server(port, job_threads_num, seconds_per_tick, auction_file_name);

    return 0;
}
