#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>

#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"
#include "../include/routing_table.h"

#include "../include/init.h"

#define ROUTER_INFO_FIELD_SIZE 0x08
#define DV_INFO_FIELD_SIZE 0x0C
//#define PRINT

void routing_table_response(int sock_index)
{
	uint16_t response_len, payload_len = ROUTER_INFO_FIELD_SIZE * TOTAL_ROUTERS;
	char *cntrl_response_header, *cntrl_response, *cntrl_response_payload;

	cntrl_response_payload = create_routing_table_payload(payload_len);

	//printf("payload_len = %d\n", payload_len);
	cntrl_response_header = create_response_header(sock_index, 2, 0, payload_len);

	response_len = CNTRL_RESP_HEADER_SIZE + payload_len;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);

	/* Copy Payload */
	memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
	free(cntrl_response_payload);
	sendALL(sock_index, cntrl_response, response_len);
	free(cntrl_response);
}

char* create_routing_table_payload(int payload_len)
{
	char *payload = (char *) malloc(payload_len);
	memset(payload, 0, sizeof(payload));
	int it = 0;

	for(int i = 1; i <= TOTAL_ROUTERS; i++)
	{
		struct ROUTING_REPONSE_ROUTER_INFO router_payload;// = (struct ROUTING_REPONSE_ROUTER_INFO *) malloc(sizeof(char) * ROUTER_INFO_FIELD_SIZE);
		router_payload.router_id = routing_table[0][i];
		router_payload.router_id = htons(router_payload.router_id);
		router_payload.padding = 0;
		router_payload.nexthop_id = nexthop_table[i-1][1];
		router_payload.nexthop_id = htons(router_payload.nexthop_id);
		router_payload.cost = routing_table[MYROUTER_INDEX][i];
		router_payload.cost = htons(router_payload.cost);
		memcpy(payload + (it * ROUTER_INFO_FIELD_SIZE), &router_payload, ROUTER_INFO_FIELD_SIZE);
		it++;
		//free(router_payload);
	}

	FILE* pFile;
	for(int i = 0; i <= TOTAL_ROUTERS; i++)
	{
		for(int j = 0; j <= TOTAL_ROUTERS; j++)
		{
			printf("%d\t", routing_table[i][j]);
		}
		printf("\n");
	}
	
	return payload;
}

char* create_dv_payload(int payload_len)
{
	char *payload = (char *) malloc(payload_len);
	memset(payload, 0, sizeof(payload));
	uint16_t num_routers = ntohs(NUM_ROUTERS);
	uint16_t router_p = ntohs(ROUTER_PORT);
	//uint32_t router_i;
	struct sockaddr_in sa;
	printf("MY_IP = %s\n", MY_IP);
	inet_pton(AF_INET, MY_IP, &(sa.sin_addr));
	memcpy(payload, &num_routers, sizeof(uint16_t));
	memcpy(payload + sizeof(uint16_t), &router_p, sizeof(uint16_t));
	memcpy(payload + sizeof(uint16_t) + sizeof(uint16_t), &(sa.sin_addr), sizeof(struct in_addr));

	int it = 0;
	for(int i = 1; i <= TOTAL_ROUTERS; i++)
	{
		struct DV_REPONSE_ROUTER_INFO dv_payload;
		//printf("routing_table[0][i] = %d\n",routing_table[0][i]);
		dv_payload.router_id = routing_table[0][i];
		dv_payload.router_id = htons(dv_payload.router_id);
		struct router_list *temp = r_list_head;
		while(temp != NULL)
		{
			if(ntohs(temp->router.router_id) == routing_table[0][i])
			{
				dv_payload.router_port = temp->router.router_port;
				dv_payload.router_ip = temp->router.router_ip;
				//printf("dv_payload.router_port = %d routing_table[0][i] = %d\n", ntohs(dv_payload.router_port), routing_table[0][i]);
				break;
			}
			temp = temp->next;
		}
		dv_payload.padding = 0;
		dv_payload.cost = routing_table[MYROUTER_INDEX][i];
		dv_payload.cost = htons(dv_payload.cost);
		//printf("SENDING dv_payload.router_port = %d dv_payload.router_id = %d dv_payload.cost = %d\n", ntohs(dv_payload.router_port),ntohs(dv_payload.router_id), ntohs(dv_payload.cost));
		memcpy(payload + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + (it * DV_INFO_FIELD_SIZE), &dv_payload, DV_INFO_FIELD_SIZE);
		it++;
	}
	/*FILE *test;
        char filename[100];
        strcpy(filename, MY_IP);
        strcat(filename, "dump");
        test = fopen(filename, "a");
        fwrite(payload, (DV_INFO_FIELD_SIZE * 5) + 8, 1, test);
        fclose(test);*/
	return payload;
}

void send_dv()
{
	struct router_list *temp = r_list_head;
	uint16_t payload_len = (DV_INFO_FIELD_SIZE * TOTAL_ROUTERS) + 8;
        char *r_table_payload;
        r_table_payload = create_dv_payload(payload_len);
	FILE* pFile;

	while(temp)
	{
		if(ntohs(temp->router.router_id) != MYROUTER_ID && ntohs(temp->router.cost) != 65535)
		{

			int sock = socket(AF_INET, SOCK_DGRAM, 0);
			struct sockaddr_in remote_address;
			socklen_t  len = sizeof(struct sockaddr);
	
			remote_address.sin_family = AF_INET;;
			remote_address.sin_port = temp->router.router_port;
			remote_address.sin_addr.s_addr = temp->router.router_ip;
			int lentes = sendtoALL(sock, r_table_payload, payload_len, 0,(struct sockaddr *)&remote_address, sizeof(remote_address));
			printf("payload_len = %d lentes = %d\n", payload_len, lentes);
			close(sock);

		}
		temp = temp->next;
	}
	free(r_table_payload);
	printf("DV sent\n");
}

void parse_dv(char *payload)
{
	FILE *pFile;
	/*FILE *test;
	char filename[100];
	strcpy(filename, MY_IP);
	strcat(filename, "recvdump");
	test = fopen(filename, "a");
	fwrite(payload, (DV_INFO_FIELD_SIZE * 5) + 8, 1, test);
	fclose(test);*/


/*#if defined PRINT
        WriteFormatted(pFile, "Routing table before parsing dv\n");
#endif
        for(int i = 0; i <= TOTAL_ROUTERS; i++)
        {
                for(int j = 0; j <= TOTAL_ROUTERS; j++)
                {
                        printf("%d\t", routing_table[i][j]);
#if defined PRINT
                        WriteFormatted(pFile, "%d\t", routing_table[i][j]);
#endif
                }
                printf("\n");
#if defined PRINT
                WriteFormatted(pFile, "\n");
#endif
	}
*/

	printf("DV received. Parsing DV\n");
	/*int router_id, router_index;
	for(int i = 0; i < TOTAL_ROUTERS; i++)
	{
		struct ROUTING_REPONSE_ROUTER_INFO router_payload;
		memcpy(&router_payload, payload + (i * ROUTER_INFO_FIELD_SIZE), ROUTER_INFO_FIELD_SIZE);
		router_payload.router_id = ntohs(router_payload.router_id);
		router_payload.nexthop_id = ntohs(router_payload.nexthop_id);
		router_payload.cost = ntohs(router_payload.cost);
		if(router_payload.cost == 0)
		{
			router_id = router_payload.router_id;
			break;
		}
	}
	for(int i = 1; i <= TOTAL_ROUTERS; i++)
	{
		if(routing_table[i][0] == router_id)
		{
			router_index = i;
			routing_table[router_index][0] = router_id;
			break;
		}
	}

#if defined PRINT
	WriteFormatted(pFile,"Router Index = %d\n", router_index);
#endif
	for(int i = 1; i <= TOTAL_ROUTERS; i++)
	{
		struct ROUTING_REPONSE_ROUTER_INFO router_payload;
		memcpy(&router_payload, payload + ((i - 1) * ROUTER_INFO_FIELD_SIZE), ROUTER_INFO_FIELD_SIZE);
		routing_table[router_index][i] = ntohs(router_payload.cost);
	}*/

	int router_id, router_index;
	uint16_t num_r, source_r_port;
	memcpy(&num_r, payload, sizeof(num_r));
	printf("num_r = %d\n", ntohs(num_r));
	memcpy(&source_r_port, payload + sizeof(num_r), sizeof(num_r));
	uint32_t s_ip;
	memcpy(&s_ip, payload + sizeof(num_r) + sizeof(num_r), sizeof(s_ip));
	char client_IP[100];
	inet_ntop(AF_INET, &(s_ip), client_IP, sizeof(client_IP));
	printf("Source ip = %s source_r_port = %d\n", client_IP, ntohs(source_r_port));
	for(int i = 0; i < TOTAL_ROUTERS; i++)
	{
		struct DV_REPONSE_ROUTER_INFO dv_payload;
		memcpy(&dv_payload, payload + 8 + (i * DV_INFO_FIELD_SIZE), DV_INFO_FIELD_SIZE);
		dv_payload.router_id = ntohs(dv_payload.router_id);
		dv_payload.cost = ntohs(dv_payload.cost);
		char clienttest[100];
		inet_ntop(AF_INET, &(dv_payload.router_ip), clienttest, sizeof(clienttest));
		printf("IP = %s\n", clienttest);
		//printf("dv_payload.router_id = %d\n", dv_payload.router_id);
		printf("FIRST dv_payload.router_port = %d dv_payload.router_id = %d dv_payload.cost = %d\n", ntohs(dv_payload.router_port), (dv_payload.router_id), (dv_payload.cost));
		if(dv_payload.cost == 0)
		{
			router_id = dv_payload.router_id;
			//break;
		}
	}

	printf("router_id = %d\n", router_id);
	for(int i = 1; i <= TOTAL_ROUTERS; i++)
	{
		if(routing_table[i][0] == router_id)
		{
			router_index = i;
			routing_table[router_index][0] = router_id;
			break;
		}
	}
	printf("router_index = %d\n",router_index);
	for(int i = 1; i <= TOTAL_ROUTERS; i++)
	{
		struct DV_REPONSE_ROUTER_INFO dv_payload;
		memcpy(&dv_payload, payload + 8 + ((i - 1) * DV_INFO_FIELD_SIZE), DV_INFO_FIELD_SIZE);
		dv_payload.router_port = ntohs(dv_payload.router_port);
		dv_payload.router_id = ntohs(dv_payload.router_id);
		dv_payload.cost = ntohs(dv_payload.cost);
		printf("dv_payload.router_port = %d dv_payload.router_id = %d dv_payload.cost = %d\n", dv_payload.router_port, dv_payload.router_id, dv_payload.cost);
		if(crashed_routers[i - 1] == 0)
			routing_table[router_index][i] = (dv_payload.cost);
	}
/*	printf("Routing table received before bellman ford\n");
#if defined PRINT
	//if(MY_IP && !strcmp(MY_IP, "128.205.36.46"))
		WriteFormatted(pFile, "Routing table received before bellman ford\n");
#endif
	for(int i = 0; i <= TOTAL_ROUTERS; i++)
	{
		for(int j = 0; j <= TOTAL_ROUTERS; j++)
		{
			printf("%d\t", routing_table[i][j]);
#if defined PRINT
			//if(MY_IP && !strcmp(MY_IP, "128.205.36.46"))
				WriteFormatted(pFile, "%d\t", routing_table[i][j]);
#endif
		}
		printf("\n");
#if defined PRINT
		//if(MY_IP && !strcmp(MY_IP, "128.205.36.46"))
			WriteFormatted(pFile, "\n");
#endif
	}*/
	run_bellman_ford();


 printf("Routing table after bellman\n");
        for(int i = 0; i <= TOTAL_ROUTERS; i++)
        {
                for(int j = 0; j <= TOTAL_ROUTERS; j++)
                {
                        printf("%d\t", routing_table[i][j]);
                }
                printf("\n");
        }

        printf("Nexthop table after bellman\n");
        for(int i = 0; i < TOTAL_ROUTERS; i++)
        {
                for(int j = 0; j < 2; j++)
                {
                        printf("%d\t", nexthop_table[i][j]);
                }
                printf("\n");
        }

	struct timer_map *it = timer_head;
	bool found = FALSE;
	while(it != NULL)
	{
		if(it->router_id == router_id)
		{
			found = TRUE;
			break;
		}
		it = it->next;
	}

	if(found)
	{
		/*struct timeval cur_time;
		gettimeofday(&cur_time, NULL);
		it->wakeat.tv_sec = cur_time.tv_sec + UPDATE_INTERVAL;
		it->wakeat.tv_usec = cur_time.tv_usec;
		/*time_t timer = time(NULL);
 		timer_head->wakeat = (long)timer + UPDATE_INTERVAL;
 		printf("timer = %ld\n", (long)timer);*/

		struct timer_map* tempNode, *prev;
		tempNode = timer_head;
		
		if (tempNode != NULL && tempNode->router_id == router_id)
		{
			timer_head = tempNode->next;
			free(tempNode);
		}
		else
		{
			while (tempNode != NULL && tempNode->router_id != router_id)
			{
				prev = tempNode;
				tempNode = tempNode->next;
			}

			if(tempNode != NULL)
			{
				prev->next = tempNode->next;
				free(tempNode);
			}
		}
		
	}
	//else
	{
		struct timer_map *temp = malloc(sizeof(struct timer_map));
		struct timeval cur_time;
		gettimeofday(&cur_time, NULL);
		temp->wakeat.tv_sec = cur_time.tv_sec + UPDATE_INTERVAL;
		temp->wakeat.tv_usec = cur_time.tv_usec;
		temp->router_id = router_id;
		temp->noresponse = 0;
		temp->next = NULL;
		it = timer_head;

		if(timercmp(&(it->wakeat), &(temp->wakeat), >=))
		{
			temp->next = it;
			it = temp;
		}
		else
		{
			while (it->next != NULL && timercmp(&(it->next->wakeat), &(temp->wakeat), <))
			{
				it = it->next;
			}
			temp->next = it->next;
			it->next = temp;
		}
	}

	/*it = timer_head;
	while(it != NULL)
	{
		printf("it->wakeat.tv_sec = %ld it->wakeat.tv_usec = %ld it->router_id = %d\n", it->wakeat.tv_sec, it->wakeat.tv_usec, it->router_id);
		it = it->next;
	}*/



}

void run_bellman_ford(bool update)
{
	FILE *pFile;
	//printf("RUN BELL\n");
	for(int i = 1; i <= TOTAL_ROUTERS; i++)
	{
		int min = routing_table[MYROUTER_INDEX][i];
		//int min = 65535;
		int nexthop_index;
		if(routing_table[0][i] == MYROUTER_ID)
			continue;

		for(int j = 1; j <= TOTAL_ROUTERS; j++)
		{
			int temp = routing_table[MYROUTER_INDEX][j] + routing_table[j][i];
			if(temp <= min && j != MYROUTER_INDEX)
			{
				min = temp;
				nexthop_index = j;
			}
		}
		if(min != routing_table[MYROUTER_INDEX][i])
		{
			routing_table[MYROUTER_INDEX][i] = min;
			if(nexthop_table[nexthop_index - 1][1] == routing_table[0][nexthop_index])
				nexthop_table[i - 1][1] = routing_table[0][nexthop_index];
			else
				nexthop_table[i - 1][1] = nexthop_table[nexthop_index - 1][1];

			if(min == 65535)
				nexthop_table[i - 1][1] = 65535;
		}
		/*else
		{
			//added to handle crash propagation.
			int nexthop_index;
			int nexthop_id = nexthop_table[i - 1][1];
			for(int k = 1; k <= TOTAL_ROUTERS; k++)
			{
				if(routing_table[0][k] == nexthop_id)
				{
					nexthop_index = k;
					break;
				}
			}
			if(routing_table[3][i] == 65535)
				routing_table[MYROUTER_INDEX][i] = 65535;
		}*/
	}

}
