/*******************************************************************************
* SOURCE FILE: server.c
*
* PROGRAM: Covert
*
* FUNCTIONS:
* void doDecoding(unsigned short port, char* file_name, int type);
*
* DATE: September 13, 2012
*
* DESIGNER: Karl Castillo (c)
*
* PROGRAMMER: Karl Castillo (c)
*
* NOTES:
* The server side of the covert program.
*
* COMPILE:
* gcc -W -Wall -o server server.c
*******************************************************************************/
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

/* STRUCTURES */
typedef struct recvdhr {
        struct iphdr ip;
        struct tcphdr tcp;
        char buffer[10000];
} RECVHDR, *PRECVHDR;

/* PROTOTYPES */
void doDecoding(unsigned int source, unsigned short port, char* file_name, int type);
unsigned int ip_convert(char *hostname);

/*******************************************************************************
* FUNCTION: main
*
* DATE: September 13, 2012
*
* REVISIONS: (Date and Description)
*
* DESIGNER: Karl Castillo (c)
*
* PROGRAMMER: Karl Castillo (c)
*
* INTERFACE: int main(int argc, char* argv[])
* argc: the number of arguments including the name of the program
* argv: the array containing the arguments where arg[0] is the name of the
*       program
*
* RETURN: int
* 0: in success
* 1: not running in root error
* 2: no decoding type chosen
*
* NOTES:
* A generic main function where the command line arguments are parsed, and the
* proper preparations are made.
*******************************************************************************/
int main(int argc, char* argv[])
{
	unsigned int source_ip = 0;
        unsigned short port = DEF_PORT;
        int encoding_type = 0;
        int option = 0;
        char* encoding_name;
        char file_name[80] = DEF_FIL;
        
        if(getuid() != 0) {
                perror("\nYou must run this in root!\n");
                return 1;
        }
        
        while((option = getopt(argc, argv, ":s:f:utl")) != -1) {
                switch(option) {
                case 'S': /* source IP */
                	source_ip = ip_convert(optarg);
                        break;
                case 's': /* source port */
                        port = atoi(optarg);
                        break;
                case 'f': /* file name */
                        strncpy(file_name, optarg, 79);
                        break;
                case 't': /* TOS */
                        encoding_type = TOS;
                        encoding_name = "TOS";
                        break;
                case 'l': /* TTL */
                        encoding_type = TTL;
                        encoding_name = "TTL";
                        break;
                }
        }
        
        if(encoding_type == 0) {
                printf("Please select an encoding type (-u or -w)\n");
                return 2;
        }
        
        printf("Covert Data Transfer using TCP Version %s (Server)\n", VERSION);
        printf("Karl Castillo (c)\n\n");
        printf("File Name: %s\n", file_name);
        printf("Encoding: %s\n", encoding_name);
        
        doDecoding(source_ip, port, file_name, encoding_type);
        
        return 0;
}

/*******************************************************************************
* FUNCTION: doDecoding
*
* DATE: September 13, 2012
*
* REVISIONS: (Date and Description)
*
* DESIGNER: Karl Castillo (c)
*
* PROGRAMMER: Karl Castillo (c)
*
* INTERFACE: void doDecoding(unsigned short port, char* file_name, int type)
* port: the port where the data will be coming from
* file_name: the name of the output file where the data will be written in
* type: the type of encoding
*       1: TOS
*       2: TTL
*
* RETURN: void
*
* NOTES:
* DoDecoding is where the server reads from the socket and properly decodes
* the packet for writing in the file. The data might be stored in the TOS field,
* or TTL.
*******************************************************************************/
void doDecoding(unsigned int source, unsigned short port, char* file_name, int type)
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
                
                printf("printing2\n");
                if(recvhdr->tcp.syn == 1 && recvhdr->ip.saddr == source) {
                	printf("printing\n");
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
        }
        free(recvhdr);
        fclose(file);
}

/*******************************************************************************
* FUNCTION: ip_convert
*
* DATE: September 13, 2012
*
* REVISIONS: (Date and Description)
*
* DESIGNER: Karl Castillo (c)
*
* PROGRAMMER: Karl Castillo (c)
*
* INTERFACE: unsigned int ip_convert(char *hostname)
* hostname: the ip address that will be converted
*
* RETURN: unsigned int: converted hostname
*
* NOTES:
* Converts a hostname to the proper format for the socket
*******************************************************************************/
unsigned int ip_convert(char *hostname)
{
        static struct in_addr inaddr;
        struct hostent *host;
        
        if((inaddr.s_addr = inet_addr(hostname)) == 0) {
                if((host = gethostbyname(hostname)) == NULL) {
                        fprintf(stderr, "Cannot resolve %s\n", hostname);
                        exit(3);
                }
                bcopy(host->h_addr, (char*)&inaddr.s_addr, host->h_length);
        }
        return inaddr.s_addr;
}
