#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <semaphore.h>

#define NM_PORT_SS 5566
#define NM_PORT_Client 5567
#define IP "127.0.0.1"

typedef struct LRU{
  char client_command[1024];
  int port;
}LRU;

