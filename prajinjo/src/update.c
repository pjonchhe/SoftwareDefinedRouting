#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "../include/update.h"
#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"
#include "../include/routing_table.h"

#include "../include/init.h"

//#define PRINT

void process_update_payload(char* payload)
{
	uint16_t r_id, cost;
	memcpy(&r_id, payload, sizeof(r_id));
	memcpy(&cost, payload + sizeof(r_id), sizeof(cost));
	r_id = ntohs(r_id);
	cost = ntohs(cost);
	for(int i = 1; i <= TOTAL_ROUTERS; i++)
	{
		if(routing_table[0][i] == r_id)
		{
			routing_table[MYROUTER_INDEX][i] = cost;
			break;
		}
	}

	for(int i = 0; i <= TOTAL_ROUTERS; i++)
	{
		for(int j = 0; j <= TOTAL_ROUTERS; j++)
		{
			printf("%d\t", routing_table[i][j]);
		}
		printf("\n");
	}
	run_bellman_ford();
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



}

void update_response(int sock_index)
{
	uint16_t response_len, payload_len = 0;
	char *cntrl_response_header, *cntrl_response;
	
	cntrl_response_header = create_response_header(sock_index, 3, 0, payload_len);

	response_len = CNTRL_RESP_HEADER_SIZE + payload_len;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);

	sendALL(sock_index, cntrl_response, response_len);

	free(cntrl_response);
}
