#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define SA struct sockaddr

void run_server(int server_port, int number_of_job_threads, int seconds_per_tick, char* auction_file);
void run_client(char *server_addr_str, int server_port);

#define USAGE_MSG "./bin/zbid_server [-h] [-j N] [-t M] PORT_NUMBER AUCTION_FILENAME\n\n-h                 Displays this help menu, and returns EXIT_SUCCESS.\n-j N               Number of job threads. If option not speicfied, default is to be in debug mode and to only tick upon input from stdin e.g. a newline is entered then the server ticks once.\nPORT_NUMBER        Port number to listen on.\nAUCTION_FILENAME   File to read auction item information from at the start of the server."

#endif
