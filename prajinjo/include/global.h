#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>


typedef enum {FALSE, TRUE} bool;

#define ERROR(err_msg) {perror(err_msg); exit(EXIT_FAILURE);}

/* https://scaryreasoner.wordpress.com/2009/02/28/checking-sizeof-at-compile-time/ */
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)])) // Interesting stuff to read if you are interested to know how this works

#define TOTAL_ROUTERS 5
#define DATAPKT_HEADER_SIZE 0x0C

uint16_t CONTROL_PORT;
uint16_t NUM_ROUTERS;
uint16_t UPDATE_INTERVAL;
uint16_t ROUTER_PORT;
uint16_t DATA_PORT;

int MYROUTER_ID;
int MYROUTER_INDEX;

char MY_IP[20];
char *last_data_packet;
char *penultimate_data_packet;

/*Used in INIT control message to parse payload*/
struct __attribute__((__packed__)) INIT_ROUTER_INFO
{
	uint16_t router_id;
	uint16_t router_port;
	uint16_t data_port;
	uint16_t cost;
	uint32_t router_ip;
};

struct router_list
{
	struct INIT_ROUTER_INFO router;
	struct router_list *next;
};

struct __attribute__((__packed__)) ROUTING_REPONSE_ROUTER_INFO
{
	uint16_t router_id;
	uint16_t padding;
	uint16_t nexthop_id;
	uint16_t cost;
};

struct __attribute__((__packed__)) DV_REPONSE_ROUTER_INFO
{
	uint32_t router_ip;
	uint16_t router_port;
	uint16_t padding;
	uint16_t router_id;
	uint16_t cost;
};

struct router_list *r_list_head, *r_list_tail;

fd_set master_list, watch_list;
int head_fd;

//int index_map[TOTAL_ROUTERS];
int routing_table[TOTAL_ROUTERS + 1][TOTAL_ROUTERS + 1];
int nexthop_table[TOTAL_ROUTERS][2];
int neighbours[TOTAL_ROUTERS];
int crashed_routers[TOTAL_ROUTERS];

int NUM_NEIGHBOUR;

struct timer_map
{
	int router_id;
	struct timeval wakeat;
	//int wakeat;
	struct timer_map *next;
	int noresponse;
}*timer_head;

struct stats
{
	struct stats* next;
	uint8_t transfer_id;
	uint8_t ttl;
	uint16_t first_seqnum;
	uint16_t last_seqnum;
}* stats_list;
#endif
