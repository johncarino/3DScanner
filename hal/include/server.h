#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <sys/types.h>

int is_blank(const char *s);
void trim(char *s);
int send_to_client(const void *buf, size_t len, const struct sockaddr *p, socklen_t pl);
struct sockaddr_in getcliaddr();
void server_init(void);
void *runServer(void *arg);
void server_cleanup(void);

#endif