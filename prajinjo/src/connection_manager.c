#include <sys/select.h>
#include <string.h>
#include <sys/time.h>

#include "../include/connection_manager.h"
#include "../include/global.h"
#include "../include/control_handler.h"
#include "../include/routing_table.h"
#include "../include/network_util.h"
#include "../include/data_handler.h"
#include "../include/init.h"

/*fd_set master_list, watch_list;
int head_fd;*/
#define DV_INFO_FIELD_SIZE 0x0C
//#define PRINT
void main_loop()
{
	int selret, sock_index, fdaccept;
	struct timeval tv, cur_time;

	while(TRUE){
		watch_list = master_list;
		if(timer_head != NULL)
		{
			/*time_t timer = time(NULL);
			tv.tv_sec = timer_head->wakeat - (long)timer;
			if(tv.tv_sec < 0)
				tv.tv_sec = 0;*/

			gettimeofday(&cur_time, NULL);
			/*.tv_sec = timer_head->wakeat.tv_sec - cur_time.tv_sec;
			if(tv.tv_sec < 0)
				tv.tv_sec = 0;
			if(tv.tv_sec > UPDATE_INTERVAL)
				tv.tv_sec = UPDATE_INTERVAL;*/
			//printf("wakeat.tv_sec = %ld wakeat.tv_usec = %ld cur_time.tv_sec = %ld cur_time.tv_usec = %ld\n", timer_head->wakeat.tv_sec, timer_head->wakeat.tv_usec, cur_time.tv_sec, cur_time.tv_usec);

			if (timer_head->wakeat.tv_sec > cur_time.tv_sec)
			{
				timersub (&timer_head->wakeat, &cur_time, &tv);
			}
			else if (timer_head->wakeat.tv_sec < cur_time.tv_sec)
			{
				tv.tv_sec = 0;
				tv.tv_usec = 0;
			}
			else
			{
				if (timer_head->wakeat.tv_usec >= cur_time.tv_usec)
					timersub (&timer_head->wakeat, &cur_time, &tv);
				else
				{
					tv.tv_sec = 0;
					tv.tv_usec = 0;
				}
			}
			
			//tv.tv_usec = timer_head->wakeat.tv_usec - cur_time.tv_usec;
			/*if((tv.tv_sec * 1000000 + tv.tv_usec) < 0)
			{
				tv.tv_usec = 0;
				tv.tv_sec = 0;
			}*/
			//printf("timer_head->wakeat = %ld timer = %ld\n",timer_head->wakeat,(long)timer);
			//printf("select with time %ld %ld", (tv.tv_sec), tv.tv_usec);
			selret = select(head_fd+1, &watch_list, NULL, NULL, &tv);
		}
		else
			selret = select(head_fd+1, &watch_list, NULL, NULL, NULL);

		if(selret < 0)
		{
			ERROR("select failed.");
		}
		else if(selret == 0)
		{
			//printf("Timer out happened!!\n");
			/*time_t timer = time(NULL);
			if(timer_head->router_id == MYROUTER_ID)
			{
				send_dv();
				timer_head->wakeat = UPDATE_INTERVAL + (long)timer;
				printf("after timeout timer_head->wakeat = %ld timer = %ld\n",timer_head->wakeat,(long)timer);
				continue;
			}*/
			struct timeval cur_temp;
			gettimeofday(&cur_temp, NULL);
			if(timer_head->router_id == MYROUTER_ID)
			{
				send_dv();
			}
				/*timer_head->wakeat.tv_sec = cur_temp.tv_sec + UPDATE_INTERVAL;
				timer_head->wakeat.tv_usec = cur_temp.tv_usec;
				continue;
			}
			else*/
			{
				struct timer_map* temp = timer_head;
				timer_head = temp->next;
				temp->wakeat.tv_sec = cur_temp.tv_sec + UPDATE_INTERVAL;
				temp->wakeat.tv_usec = cur_temp.tv_usec;
				if(temp->router_id != MYROUTER_ID)
					temp->noresponse++;
				else
					temp->noresponse = 0;

				if(temp->noresponse < 3)
				{
					struct timer_map *it = timer_head;
					if(it != NULL)
					{
						while(it->next != NULL)
							it = it->next;
						it->next = temp;
						temp->next = NULL;
					}
					else
						timer_head = temp;
				}
				else
				{
					printf("ROUTER CRASHED id = %d\n", temp->router_id);
					int i;
					for(i = 1; i <= TOTAL_ROUTERS; i++)
					{
						if(routing_table[0][i] == temp->router_id)
						{
							routing_table[MYROUTER_INDEX][i] = 65535;
							for(int k = 1; k <= TOTAL_ROUTERS; k++)
								routing_table[k][i] = 65535;

							crashed_routers[i - 1] = 1;
							break;
						}
					}
					for(int j = 1; j <= TOTAL_ROUTERS; j++)
					{
						routing_table[i][j] = 65535;
					}
					nexthop_table[i-1][1] = 65535;
					//run_bellman_ford();
					free(temp);
				}
				
			}
			continue;
		}

		/* Loop through file descriptors to check which ones are ready */
		for(sock_index=0; sock_index<=head_fd; sock_index+=1){

			if(FD_ISSET(sock_index, &watch_list)){

				/* control_socket */
				if(sock_index == control_socket){
					fdaccept = new_control_conn(sock_index);
					printf("Control socket = %d\n", fdaccept);

					/* Add to watched socket list */
					FD_SET(fdaccept, &master_list);
					if(fdaccept > head_fd) head_fd = fdaccept;
                		}

				/* router_socket */
				else if(sock_index == router_socket){
				//call handler that will call recvfrom() .....
					struct sockaddr_in client_address;
					int client_address_size = sizeof(client_address);
					uint16_t payload_len = (DV_INFO_FIELD_SIZE * TOTAL_ROUTERS) + 8;
					char *payload = (char *) malloc(payload_len);
					memset(payload, 0, sizeof(payload));
					int rec = recvfromALL(sock_index, payload, payload_len, 0, (struct sockaddr *) &client_address, &client_address_size);
					parse_dv(payload);
					free(payload);
					//printf("I received DV!!!!!!\n");
				}

				/* data_socket */
				else if(sock_index == data_socket){
					fdaccept = new_data_conn(sock_index);
					//printf("Got connection request with data socket = %d\n", fdaccept);
					FD_SET(fdaccept, &master_list);
					if(fdaccept > head_fd) head_fd = fdaccept;
				}

				/* Existing connection */
				else{
					//printf("GOT something in socket = %d\n", sock_index);
					if(isControl(sock_index)){
						if(!control_recv_hook(sock_index)) FD_CLR(sock_index, &master_list);
					}
					else if (isData(sock_index))
					{
						if(!data_recv_hook(sock_index)) FD_CLR(sock_index, &master_list);
					}
					else
					{
						printf("sock_index = %d\n", sock_index);
						ERROR("Unknown socket index");
					}
				}
			}
		}
	}
}

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec)
	{
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000)
	{
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}
	/* Compute the time remaining to wait.  tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;
	return x->tv_sec < y->tv_sec;
}

void init()
{
	control_socket = create_control_sock();
	//router_socket and data_socket will be initialized after INIT from controller
	FD_ZERO(&master_list);
	FD_ZERO(&watch_list);
	/* Register the control socket */
	FD_SET(control_socket, &master_list);
	head_fd = control_socket;

	main_loop();
}
