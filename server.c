/***********************************************************
* SOURCE FILE: epoll.cpp
*
* PROGRAM: PortForwarder
*
* FUNCTIONS:
* int Epoll_create(int queue);
* int Epoll_ctl(int epfd, int op, int sockfd, struct epoll_event *event);
* int Epoll_wait(int epollfd, struct epoll_event *event, int queue, int timeout);
*
* DATE: MARCH 11, 2012
*
* DESIGNER: Karl Castillo
*
* PROGRAMMER: Karl Castillo
*
* NOTES:
* Contains wrapper functions for the Linux ePoll API.
***********************************************************/
/* INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/ip.h>

/* DEFINES */
#define VERSION         "1.0"
#define TOS             1
#define TTL             2
#define DEF_PORT        8000
#define DEF_FIL         "secret2.txt"

typedef struct recvdhr {
        struct iphdr ip;
        struct tcphdr tcp;
        char buffer[10000];
} RECVHDR, *PRECVHDR;

void doDecoding(unsigned short port, char* file_name, int type);

int main(int argc, char* argv[])
{
        unsigned short port = DEF_PORT;
        int encoding_type = 0;
        int option = 0;
        char file_name[80] = DEF_FIL;
        
        if(getuid() != 0) {
                perror("\nYou must run this in root!\n");
                exit(EXIT_FAILURE);
        }
        
        while((option = getopt(argc, argv, ":p:f:utl")) != -1) {
                switch(option) {
                case 'p':
                        port = atoi(optarg);
                        break;
                case 'f':
                        strncpy(file_name, optarg, 79);
                        break;
                case 't':
                        encoding_type = TOS;
                        break;
                case 'l':
                        encoding_type = TTL;
                        break;
                }
        }
        
        if(encoding_type == 0) {
                printf("Please select an encoding type (-u or -w)\n");
                return 1;
        }
        
        doDecoding(port, file_name, encoding_type);
        
        return 0;
}

void doDecoding(unsigned short port, char* file_name, int type)
{
        PRECVHDR recvhdr = (PRECVHDR)malloc(sizeof(RECVHDR));
        FILE *file;
        int sock;
        
        if((file = fopen(file_name, "wb")) == NULL) {
                fprintf(stderr, "Cannot open %s", file_name);
                exit(1);
        }
        
        while(1) {
                if((sock = socket(AF_INET, SOCK_RAW, 6)) < 0) {
                        perror("Cannot open socket");
                        exit(2);
                }
                
                read(sock, recvhdr, 9999);
                
                if(type == TOS) { /* data in TOS field */
                        printf("Receiving data: %c\n", recvhdr->ip.tos);
                        fprintf(file, "%c", recvhdr->ip.tos);
                        fflush(file);
                } else if(type == TTL) { /* data found in TTL */
                        printf("Receiving data: %c\n", 
                                (char)(recvhdr->ip.ttl - 64));
                        fprintf(file, "%c", (char)(recvhdr->ip.ttl - 64));
                        fflush(file);
                }
                close(sock);
        }
        free(recvhdr);
        fclose(file);
}
