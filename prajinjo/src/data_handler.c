#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <sys/queue.h>
#include <arpa/inet.h>

#include "../include/global.h"
#include "../include/network_util.h"
#include "../include/control_header_lib.h"
#include "../include/author.h"
#include "../include/init.h"
#include "../include/routing_table.h"
#include "../include/connection_manager.h"
#include "../include/update.h"
#include "../include/sendfile.h"
#include "../include/data_handler.h"
#define DATA_HEADER_SIZE 12

//#define PRINT

/* Linked List for active control connections */
struct DataConn
{
        int sockfd;
        LIST_ENTRY(DataConn) next;
}*connection, *conn_temp;
LIST_HEAD(DataConnsHead, DataConn) data_conn_list;

int create_data_sock()
{
        int sock;
        struct sockaddr_in data_addr;
        socklen_t addrlen = sizeof(data_addr);

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock < 0)
                ERROR("socket() failed");

        bzero(&data_addr, sizeof(data_addr));
        data_addr.sin_family = AF_INET;
        data_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        data_addr.sin_port = htons(DATA_PORT);
        printf("DATA_PORT = %d\n", DATA_PORT);

        if(bind(sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0)
                ERROR("bind() failed");

        if(listen(sock, 5) < 0)
                ERROR("listen() failed");

	LIST_INIT(&data_conn_list);
        return sock;
}

int new_data_conn(int sock_index)
{
	int fdaccept, caddr_len;
	struct sockaddr_in remote_data_addr;
	caddr_len = sizeof(remote_data_addr);
	fdaccept = accept(sock_index, (struct sockaddr *)&remote_data_addr, &caddr_len);
	if(fdaccept < 0)
		ERROR("accept() failed");

	/* Insert into list of active control connections */
	connection = malloc(sizeof(struct DataConn));
	connection->sockfd = fdaccept;
	LIST_INSERT_HEAD(&data_conn_list, connection, next);

	return fdaccept;
}

bool isData(int sock_index)
{
	LIST_FOREACH(connection, &data_conn_list, next)
	if(connection->sockfd == sock_index) return TRUE;

	return FALSE;
}

void remove_data_conn(int sock_index)
{
	LIST_FOREACH(connection, &data_conn_list, next) {
		if(connection->sockfd == sock_index) LIST_REMOVE(connection, next); // this may be unsafe?
		free(connection);
        }

	close(sock_index);
}

bool data_recv_hook(int sock_index)
{
	int len = DATAPKT_HEADER_SIZE + 1024;
	char *data_header, *data_payload;
	data_header = (char *) malloc(sizeof(char)*DATA_HEADER_SIZE);
	bzero(data_header, DATA_HEADER_SIZE);
	FILE *pFile = NULL;

	int nexthopsock = 0;
	FILE *test;

	printf("Reached data_handler\n");

	struct stats* temp = (struct stats*) malloc(sizeof(struct stats));
	bool first = TRUE;

	uint8_t finbit = 0;
	while(finbit == 0)
	{
		if(recvALL(sock_index, data_header, DATA_HEADER_SIZE) < 0)
		{
			remove_data_conn(sock_index);
			free(data_header);
			return FALSE;
		}


		if(penultimate_data_packet == NULL)
		{
			penultimate_data_packet = (char*) malloc(len);
			memset(penultimate_data_packet, 0, len);
		}
		else
			memcpy(penultimate_data_packet, last_data_packet, len);

		//memcpy(last_data_packet, data_header, DATA_HEADER_SIZE);

		uint32_t destIP;
		uint8_t transfer_id, ttl;//, finbit;
		uint16_t seq_num;
		memcpy(&destIP, data_header, sizeof(uint32_t));
		memcpy(&transfer_id, data_header + sizeof(uint32_t), sizeof(uint8_t));
		memcpy(&ttl, data_header + sizeof(uint32_t) + sizeof(uint8_t), sizeof(uint8_t));
		ttl = ttl - 1;
		memcpy(&seq_num, data_header + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t), sizeof(uint16_t));
		memcpy(&finbit, data_header + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint8_t));

		if(first)
		{
			first = FALSE;
			temp->transfer_id = transfer_id;
			temp->ttl = ttl;
			temp->first_seqnum = ntohs(seq_num);
			temp->next = NULL;
		}

		if(finbit)
		{
			temp->last_seqnum = ntohs(seq_num);
		}

		memcpy(last_data_packet, &destIP, sizeof(uint32_t));
		memcpy(last_data_packet + sizeof(uint32_t), &transfer_id, sizeof(uint8_t));
		memcpy(last_data_packet + sizeof(uint32_t) + sizeof(uint8_t), &ttl, sizeof(uint8_t));
		memcpy(last_data_packet + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t), &seq_num, sizeof(uint16_t));
		memcpy(last_data_packet + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t), &finbit, sizeof(uint8_t));

		int payload_len = 1024;
		data_payload = (char *) malloc(sizeof(char)*payload_len);
		bzero(data_payload, payload_len);

		if(recvALL(sock_index, data_payload, payload_len) < 0)
		{
			remove_data_conn(sock_index);
			free(data_payload);
			return FALSE;
		}


		memcpy(last_data_packet + DATA_HEADER_SIZE, data_payload, payload_len);
	
	
		char dest_IP[100];
		//memset(dest_IP, 0, 100);
		inet_ntop(AF_INET, &(destIP), dest_IP, sizeof(dest_IP));

		if(!strcmp(dest_IP, MY_IP))
		{
			//FILE * pFile;
			char file_id[100];
			sprintf(file_id, "%d", transfer_id);
			char fileName[200];
			strcpy(fileName, "file-");
			strcat(fileName, file_id);

			if(pFile == NULL)
				pFile = fopen(fileName, "ab");

			fwrite (data_payload , sizeof(char), payload_len, pFile);

			//printf("data_payload = %s\n", data_payload);

			//fclose(pFile);
		}
		else if(ttl != 0)
		{
			int payload_len = 1024;
			printf("NotForMe\n");
			int nexthopid = getNextHopID(destIP);
			printf("Next hop is %d\n", nexthopid);
			if(nexthopsock == 0)
				nexthopsock = connect_to_next_hop(nexthopid);
			char *datapkt_header = (char *) malloc(sizeof(char)*DATAPKT_HEADER_SIZE);
			memset(datapkt_header, 0, DATAPKT_HEADER_SIZE);
			/*Destination IP*/
			memcpy(datapkt_header, &(destIP), sizeof(uint32_t));
			/*Transfer ID*/
			memcpy(datapkt_header + sizeof(uint32_t), &transfer_id, sizeof(uint8_t));
			/*TTL*/
			memcpy(datapkt_header + sizeof(uint32_t) + sizeof(uint8_t), &ttl, sizeof(uint8_t));
			printf("ttl = %d\n", ttl);
			/*Sequence Number*/
			memcpy(datapkt_header + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t), &seq_num, sizeof(uint16_t));
			printf("seqnum = %d\n", ntohs(seq_num));
			printf("finbit = %d\n", finbit);
			/*FIN bit*/
			memcpy(datapkt_header + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t), &finbit, sizeof(uint8_t));

			int datapkt_len = DATAPKT_HEADER_SIZE + payload_len;
			char* datapkt = (char *) malloc(datapkt_len);
			/* Copy Header */
			memcpy(datapkt, datapkt_header, DATAPKT_HEADER_SIZE);
			free(datapkt_header);

			/*Read from payload buffer*/
			memcpy(datapkt + DATAPKT_HEADER_SIZE, data_payload, payload_len);

			sendALL(nexthopsock, datapkt, datapkt_len);
			free(datapkt);
		

			/*FILE *test;
			char filename[100];
			strcpy(filename, MY_IP);
			strcat(filename, "dataheader");
			test = fopen(filename, "a");
			fwrite(datapkt_header, DATAPKT_HEADER_SIZE, 1, test);
			fclose(test);*/
		}
	}
	if(pFile)
		fclose(pFile);

	if(nexthopsock)
		close(nexthopsock);

	if(stats_list == NULL)
		stats_list = temp;
	else
	{
		struct stats* it = stats_list;
		while(it->next != NULL)
		{
			it = it->next;
		}
		it->next = temp;
	}
	free(data_payload);

	FILE *dumper;
	char filename[100];
	strcpy(filename, MY_IP);
	strcat(filename, "handlerlastpkt");
	dumper = fopen(filename, "w");
	fwrite(last_data_packet, len, 1, dumper);
	fclose(dumper);
	return TRUE;
}


int getNextHopID(uint32_t destIP)
{
	struct router_list *temp = r_list_head;
	while(temp != NULL)
	{
		if(temp->router.router_ip == destIP)
			break;
		temp = temp->next;
	}

	for(int i = 0; i < TOTAL_ROUTERS; i++)
	{
		if(nexthop_table[i][0] == ntohs(temp->router.router_id))
			return nexthop_table[i][0];
	}
	return 0;
}
