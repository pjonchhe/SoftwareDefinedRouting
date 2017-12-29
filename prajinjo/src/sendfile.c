#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/tcp.h>

#include "../include/global.h"
#include "../include/sendfile.h"
#include "../include/update.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"
#include "../include/routing_table.h"
#include "../include/init.h"

//#define PRINT
int process_sendfile_payload(char* payload, int len)
{
 FILE *pFile;
	uint32_t destIP;
	uint8_t ttl;
	uint8_t tid;
	uint16_t seq_num;
	int filename_len = len - 8;
	char* filename = (char*) malloc(filename_len + 1);
	//char filename[20];
	memset(filename, 0, filename_len + 1);
	memcpy(&destIP, payload, sizeof(destIP));
	memcpy(&ttl, payload + sizeof(destIP), sizeof(ttl));
	memcpy(&tid, payload + sizeof(destIP) + sizeof(ttl), sizeof(tid));
	memcpy(&seq_num, payload + sizeof(destIP) + sizeof(ttl) + sizeof(tid), sizeof(seq_num));
	memcpy(filename, payload + sizeof(destIP) + sizeof(ttl) + sizeof(tid) + sizeof(seq_num), filename_len);

	//ttl = ntohs(ttl);
	//tid = ntohs(tid);
	seq_num = ntohs(seq_num);
	printf("ttl = %d tid = %d seq_num = %d filename = %s\n", ttl, tid, seq_num, (filename));

	
	struct router_list *temp = r_list_head;
	int dest_id, nexthop_id;
	while(temp)
	{
		if(temp->router.router_ip == destIP)
		{
			dest_id = ntohs(temp->router.router_id);
			break;
		}
		temp = temp->next;
	}
	for(int i = 0; i < TOTAL_ROUTERS; i++)
	{
		if(nexthop_table[i][0] == dest_id)
			nexthop_id = nexthop_table[i][1];
	}

	printf("Next hop is %d\n", nexthop_id);
	int socket = connect_to_next_hop(nexthop_id);

	char* datapkt_header;

	transfer_file(socket, destIP, tid, ttl, seq_num, filename);

	close(socket);

	return socket;
}

void transfer_file(int socket, uint32_t destIP, uint8_t transfer_id, uint8_t ttl, uint16_t seq_num, char* fileName)
{
	FILE* pFile;
	pFile = fopen(fileName, "rb");

	printf("erron = %d", errno);
	//char *datapkt_header = create_datapkt_header(destIP, transfer_id, ttl, seq_num, TRUE);
        /*FILE *test;
        char filename[100];
        strcpy(filename, MY_IP);
        strcat(filename, "transdataheader");
        test = fopen(filename, "a");
        fwrite(datapkt_header, DATAPKT_HEADER_SIZE, 1, test);
	fclose(test);*/

	struct stats* temp = (struct stats*)malloc(sizeof(struct stats));
	temp->transfer_id = transfer_id;
	temp->ttl = ttl;
	temp->first_seqnum = seq_num;

	FILE* test;
	if(pFile)
	{
		int datapkt_len, payload_len = 1024;
		long fileSize, sentBytes = 0, seekPtr = 0;
		fseek (pFile , 0 , SEEK_END);
		fileSize = ftell (pFile);
		rewind (pFile);

		//char* buffer_file = (char*) malloc(sizeof(char)*fileSize);
		//fread(buffer_file, fileSize, 1, pFile);
		printf("Starting send\n");

		datapkt_len = DATAPKT_HEADER_SIZE + payload_len;
		char* buffer = (char*) malloc(sizeof(char)*payload_len);

		char *datapkt = (char *) malloc(datapkt_len);

		/*char* datapkt_header;
		if(payload_len == fileSize)
		{
			datapkt_header = create_datapkt_header(destIP, transfer_id, ttl, seq_num, TRUE);
		}
		else
		{
			datapkt_header = create_datapkt_header(destIP, transfer_id, ttl, seq_num, FALSE);
		}*/

		


		while(sentBytes != fileSize)
		{
/*#ifdef PRINT
			WriteFormatted(test, "sentBytes = %d fileSize = %d\n", sentBytes, fileSize);
#endif*/
			memset(buffer, 0, payload_len);
			memset(datapkt, 0, datapkt_len);
			printf("sentBytes = %d fileSize = %d\n", sentBytes, fileSize);
			//char* buffer = (char*) malloc(sizeof(char)*payload_len);
			char* datapkt_header;
			//char* *datapkt;
			sentBytes = sentBytes + payload_len;
			if(sentBytes == fileSize)
			{
				datapkt_header = create_datapkt_header(destIP, transfer_id, ttl, seq_num, TRUE);
			}
			else
			{
				datapkt_header = create_datapkt_header(destIP, transfer_id, ttl, seq_num, FALSE);
			}

			/*if(sentBytes == fileSize)
			{
				datapkt_header = update_datapkt_header(datapkt_header, seq_num, TRUE);
			}
			else
			{
				datapkt_header = update_datapkt_header(datapkt_header, seq_num, FALSE);
			}*/
			//datapkt_len = DATAPKT_HEADER_SIZE + payload_len;
			//datapkt = (char *) malloc(datapkt_len);
			printf("Header created\n");

			/*Copy header*/
			memcpy(datapkt, datapkt_header, DATAPKT_HEADER_SIZE);
			free(datapkt_header);

			/*Read from file to buffer*/
			//fseek (pFile, seekPtr, SEEK_SET);
			int readret = fread (buffer, payload_len, 1, pFile);
			//memcpy(buffer, buffer_file + seekPtr, payload_len);
			printf("data = %s\n", buffer);	
			/*Copy payload*/
			memcpy(datapkt + DATAPKT_HEADER_SIZE, buffer, payload_len);
			//memcpy(datapkt, buffer, payload_len);
			//free(buffer);

			struct timeval cur_temp1, cur_temp2, tv;
                        gettimeofday(&cur_temp1, NULL);
			sendALL(socket, datapkt, datapkt_len);

			/*Copy packet to last sent packet*/
			if(penultimate_data_packet == NULL)
			{
				penultimate_data_packet = (char*) malloc(datapkt_len);
				memset(penultimate_data_packet, 0, datapkt_len);
			}
			else
				memcpy(penultimate_data_packet, last_data_packet, datapkt_len);
			memcpy(last_data_packet, datapkt, datapkt_len);
			gettimeofday(&cur_temp2, NULL);

			timersub (&cur_temp2, &cur_temp1, &tv);
			printf("tv.sec = %ld tv.usec = %ld\n", tv.tv_sec, tv.tv_usec);
			//free(datapkt);
			//seekPtr = sentBytes;
			seq_num ++;
		}
		printf("Done\n");
		temp->last_seqnum = seq_num - 1;
		temp->next = NULL;

		if(stats_list == NULL)
		{
			stats_list = temp;
		}
		else
		{
			struct stats* it = stats_list;
			while(it->next != NULL)
			{
				it = it->next;
			}
			it->next = temp;
		}
		//free(datapkt_header);
		free(datapkt);
		free(buffer);
		//free(buffer_file);
		fclose(pFile);
	}
	FILE *dumper;
        char filename[100];
        strcpy(filename, MY_IP);
        strcat(filename, "senderlastpkt");
        dumper = fopen(filename, "w");
        fwrite(last_data_packet, DATAPKT_HEADER_SIZE + 1024, 1, dumper);
        fclose(dumper);
}

char* create_datapkt_header(uint32_t destIP, uint8_t transfer_id, uint8_t ttl, uint16_t seq_num, bool fin)
{
	char *datapkt_header = (char *) malloc(sizeof(char)*DATAPKT_HEADER_SIZE);

	char client_IP[100];
	inet_ntop(AF_INET, &(destIP), client_IP, sizeof(client_IP));

	printf("Dest IP = %s\n", client_IP);
	/*Destination IP*/
	//memcpy(datapkt_header, &(addr.sin_addr), sizeof(struct in_addr));
	memcpy(datapkt_header, &destIP, sizeof(uint32_t));
	/*Transfer ID*/
	memcpy(datapkt_header + sizeof(uint32_t), &transfer_id, sizeof(uint8_t));
	/*TTL*/
	memcpy(datapkt_header + sizeof(uint32_t) + sizeof(uint8_t), &ttl, sizeof(uint8_t));
	/*Sequence Number*/
	seq_num = htons(seq_num);
	memcpy(datapkt_header + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t), &seq_num, sizeof(uint16_t));
	/*FIN bit*/
	uint8_t finbit = 0;
	if(fin)
	{
		finbit = finbit | (1 << 7);
	}
	memcpy(datapkt_header + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t), &finbit, sizeof(uint8_t));

	return datapkt_header;
}

char *update_datapkt_header(char* datapkt_header, uint16_t seq_num, bool fin)
{
	seq_num = htons(seq_num);
	memcpy(datapkt_header + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t), &seq_num, sizeof(uint16_t));
	uint8_t finbit = 0;
	if(fin)
	{
		finbit = finbit | (1 << 7);
	}
	memcpy(datapkt_header + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t), &finbit, sizeof(uint8_t));
	return datapkt_header;
}

int connect_to_next_hop(int id)
{
	int network_socket = socket(AF_INET, SOCK_STREAM, 0);
	struct router_list *temp = r_list_head;
	struct sockaddr_in next_router_address;
	next_router_address.sin_family = AF_INET;
	while(temp)
	{
		if(ntohs(temp->router.router_id) == id)
		{
			next_router_address.sin_port = temp->router.data_port;
			//next_router_address.sin_addr = (struct in_addr)temp->router.router_ip;
			char client_IP[100];
			inet_ntop(AF_INET, &(temp->router.router_ip), client_IP, sizeof(client_IP));
			printf("client_IP = %s next_router_address.sin_port = %d\n",client_IP, ntohs(next_router_address.sin_port));
			inet_pton(AF_INET, client_IP, &next_router_address.sin_addr);
			break;
		}
		temp = temp->next;
	}

	int connection_status = connect(network_socket, (struct sockaddr *) &next_router_address, sizeof(next_router_address));
	if(connection_status == -1)
	{
		printf("THere was an error\n\n");
		return 0;
	}

	int i = 1;
setsockopt( network_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&i, sizeof(i));

	return network_socket;
	
}

void send_response(int sock_index)
{
	uint16_t response_len, payload_len = 0;
	char *cntrl_response_header, *cntrl_response;
	cntrl_response_header = create_response_header(sock_index, 5, 0, payload_len);
	response_len = CNTRL_RESP_HEADER_SIZE + payload_len;
	cntrl_response = (char *) malloc(response_len);

	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);

	sendALL(sock_index, cntrl_response, response_len);
	free(cntrl_response);
}

void sendfilestats_response(int sock_index, char *payload)
{
	char *cntrl_response_header, *cntrl_response;
	uint8_t req_tid;
	memcpy(&req_tid, payload, sizeof(req_tid));
	struct stats* it = stats_list;
	while(it != NULL)
	{
		if(it->transfer_id == req_tid)
		{
			break;
		}
		it = it->next;
	}
	FILE *pFile;

	if(it)
	{
		int numOfPackets = it->last_seqnum - it->first_seqnum + 1;
		int payload_len = (numOfPackets + 2) * sizeof(uint16_t);
		cntrl_response_header = create_response_header(sock_index, 6, 0, payload_len);

		int response_len = CNTRL_RESP_HEADER_SIZE + payload_len;
		cntrl_response = (char *) malloc(response_len);
		/* Copy Header */
		memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
		free(cntrl_response_header);

		char *payload = (char *) malloc(payload_len);
		memset(payload, 0, sizeof(payload));
		memcpy(payload, &(it->transfer_id), sizeof(uint8_t));
		memcpy(payload + sizeof(uint8_t), &(it->ttl), sizeof(uint8_t));

		uint16_t init = it->first_seqnum;

		for(int i = 0; i < numOfPackets; i++)
		{
			uint16_t seq = htons(init);
			memcpy(payload + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + (i * sizeof(uint16_t)), &seq, sizeof(uint16_t));
			init++;
		}

		memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, payload, payload_len);
		free(payload);

		sendALL(sock_index, cntrl_response, response_len);
		free(cntrl_response);
	}
}

void lastdatapacket_response(int sock_index)
{
	char *cntrl_response_header, *cntrl_response;
	int response_len, payload_len = DATAPKT_HEADER_SIZE + 1024;
	cntrl_response_header = create_response_header(sock_index, 7, 0, payload_len);
	response_len = CNTRL_RESP_HEADER_SIZE + payload_len;
	cntrl_response = (char *) malloc(response_len);
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);

	memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, last_data_packet, payload_len);

	sendALL(sock_index, cntrl_response, response_len);
	free(cntrl_response);
}

void penultimate_response(int sock_index)
{
	char *cntrl_response_header, *cntrl_response;
	int response_len, payload_len = DATAPKT_HEADER_SIZE + 1024;
	cntrl_response_header = create_response_header(sock_index, 8, 0, payload_len);
	response_len = CNTRL_RESP_HEADER_SIZE + payload_len;
	cntrl_response = (char *) malloc(response_len);
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);

	memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, penultimate_data_packet, payload_len);

	sendALL(sock_index, cntrl_response, response_len);
	free(cntrl_response);
	
}
