#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdarg.h>

#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"
#include "../include/init.h"
#include "../include/routing_table.h"
#include "../include/data_handler.h"

//#define PRINT
#define CONTROL_UPDATE_INTERVAL_OFFSET 0x02
#define CONTROL_ROUTERINFO_OFFSET 0x04
#define CONTROL_ROUTERINFO_SIZE 0x0C

/*int get_router_index(int router_id)
{
	for(int i = 0; i < TOTAL_ROUTERS; i++)
	{
		if(index_map[i] == router_id)
			return i;
	}
	return -1;
}*/
void init_process_payload(char* payload)
{
	FILE *pFile;
	/*Initialize routing table with INF*/
	for(int i = 0; i <= TOTAL_ROUTERS; i++ )
	{
		for(int j = 0; j <= TOTAL_ROUTERS; j++)
			routing_table[i][j] = 65535;
	}
	for(int i = 0; i < TOTAL_ROUTERS; i++ )
	{
		for(int j = 0; j < 2; j++)
			nexthop_table[i][j] = 65535;
	}
	/*Read number of routers*/
	uint16_t num_routers = 0;
	memcpy(&num_routers, payload, sizeof(num_routers));
	NUM_ROUTERS = ntohs(num_routers);

	/*Read Updates Periodic Interval*/
	uint16_t update_interval = 0;
	memcpy(&update_interval, payload + CONTROL_UPDATE_INTERVAL_OFFSET, sizeof(update_interval));
	UPDATE_INTERVAL = ntohs(update_interval);
	printf("UPDATE_INTERVAL = %d\n", UPDATE_INTERVAL);

	/*Read payload into router_list structure.*/
	for(int i = 0; i < NUM_ROUTERS; i++)
	{
		struct router_list *temp = malloc(sizeof(struct router_list));
		memcpy(&(temp->router), payload + (CONTROL_ROUTERINFO_OFFSET + (CONTROL_ROUTERINFO_SIZE * i)), sizeof(temp->router));
		temp->next = NULL;
		char client_IP[100];
		inet_ntop(AF_INET, &(temp->router.router_ip), client_IP, sizeof(client_IP));

		if(!strcmp(client_IP, MY_IP))
		{
			ROUTER_PORT = ntohs(temp->router.router_port);
			DATA_PORT = ntohs(temp->router.data_port);
			MYROUTER_ID = ntohs(temp->router.router_id);
			routing_table[0][1] = MYROUTER_ID;
			routing_table[1][0] = routing_table[0][1];
		}
		if(r_list_head == NULL)
		{
			r_list_head = temp;
			r_list_tail = temp;
		}
		else
		{
			r_list_tail->next = temp;
			r_list_tail = temp;
		}
	}

	struct router_list *temp = r_list_head;
	int it_r = 1;
	int it_n = 0;
	int num_neighbours = 0;

	
	while(temp != NULL)
	{
		routing_table[0][it_r] = ntohs(temp->router.router_id);
		routing_table[it_r][0] = ntohs(temp->router.router_id);
		if(routing_table[0][it_r] == MYROUTER_ID)
			MYROUTER_INDEX = it_r;
		temp = temp->next;
		it_r++;
	}
	it_r = 1;
	
	/*for(int i = 0; i <= TOTAL_ROUTERS; i++)
        {
                for(int j = 0; j <= TOTAL_ROUTERS; j++)
                {
                        printf("%d\t", routing_table[i][j]);
			WriteFormatted(pFile, "%d\t", routing_table[i][j]);
                }
                printf("\n");
		WriteFormatted(pFile, "\n");
        }*/

	temp = r_list_head;
	while(temp != NULL)
	{
		routing_table[MYROUTER_INDEX][it_r] = ntohs(temp->router.cost);
		nexthop_table[it_n][0] = routing_table[0][it_r];
		if(routing_table[MYROUTER_INDEX][it_r] != 65535)
		{
			nexthop_table[it_n][1] = nexthop_table[it_n][0];
			neighbours[num_neighbours] = routing_table[0][it_r];
			num_neighbours++;
		}
		else
		{
			nexthop_table[it_n][1] = 65535;
		}

		it_r++;
		it_n++;
		temp = temp->next;
	}
	NUM_NEIGHBOUR = num_neighbours;

	for(int i = 0; i < TOTAL_ROUTERS; i++)
	{
		crashed_routers[i] = 0;
	}

	printf("Routing table received in init\n");

	for(int i = 0; i <= TOTAL_ROUTERS; i++)
	{
		for(int j = 0; j <= TOTAL_ROUTERS; j++)
		{
			printf("%d\t", routing_table[i][j]);
		}
		printf("\n");
	}

	printf("NextHop table after init\n");
	for(int i = 0; i < TOTAL_ROUTERS; i++)
	{
		for(int j = 0; j < 2; j++)
		{
			printf("%d\t", nexthop_table[i][j]);
		}
		printf("\n");
	}

	printf("Neighbour list\n");
	for(int i = 0; i < NUM_NEIGHBOUR; i++)
	{
		printf("%d\t", neighbours[i]);
	}
	printf("\n");
	timer_head = malloc(sizeof(struct timer_map));
	timer_head->router_id = MYROUTER_ID;
	timer_head->next = NULL;
	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);
	timer_head->wakeat.tv_sec = cur_time.tv_sec + UPDATE_INTERVAL;
	timer_head->wakeat.tv_usec = cur_time.tv_usec;
	/*time_t timer = time(NULL);
	timer_head->wakeat = (long)timer + UPDATE_INTERVAL;
	printf("timer = %ld\n", (long)timer);*/


	/*Create memory to store last packet sent or received.*/
	int datapkt_len = DATAPKT_HEADER_SIZE + 1024;
	last_data_packet = (char*) malloc(datapkt_len);
 
}


void init_response(int sock_index)
{
	uint16_t response_len, payload_len = 0;
	char *cntrl_response_header, *cntrl_response, *cntrl_response_payload;

	//payload_len = sizeof(PRAJIN)-1; // Discount the NULL chararcter
	//cntrl_response_payload = (char *) malloc(payload_len);
	//memcpy(cntrl_response_payload, PRAJIN, payload_len);

	cntrl_response_header = create_response_header(sock_index, 1, 0, payload_len);

	response_len = CNTRL_RESP_HEADER_SIZE + payload_len;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);

	/* Copy Payload */
	//memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
	//free(cntrl_response_payload);

	sendALL(sock_index, cntrl_response, response_len);

	free(cntrl_response);
	//send_dv();
}

void get_myIP()
{
	int dummy_socket;
	char str[128];
	struct sockaddr_in local_address;

	dummy_socket = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in server_address;
	socklen_t  len = sizeof(struct sockaddr);

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(53);
	inet_pton(AF_INET, "8.8.8.8", &server_address.sin_addr.s_addr);

	int connection_status = connect(dummy_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	if(connection_status == -1)
	{
		printf("There was an error\n");
	}

	getsockname(dummy_socket, (struct sockaddr *)&local_address, &len);
	inet_ntop(AF_INET, &local_address.sin_addr, str, sizeof(str));

	strcpy(MY_IP, str);

	printf("MY IP = %s\n", MY_IP);
	FILE *pFile;
}

int create_router_sock()
{
	int sock;
	struct sockaddr_in router_addr;
	socklen_t len = sizeof(router_addr);

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&router_addr, sizeof(router_addr));

	router_addr.sin_family = AF_INET;
	router_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	router_addr.sin_port = htons(ROUTER_PORT);
	//printf("ROUTER_PORT = %d\n", ROUTER_PORT);
	FILE *pFile;

	if(bind(sock, (struct sockaddr *)&router_addr, sizeof(router_addr)) < 0)
	{
		ERROR("bind() failed");
	}
	return sock;
}
