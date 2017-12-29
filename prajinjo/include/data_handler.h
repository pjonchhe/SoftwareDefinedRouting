#ifndef DATA_HANDLER_H_
#define DATA_HANDLER_H_

/*struct __attribute__((__packed__)) DATA_HEADER
{
	uint32_t dest_ip_addr;
	uint8_t transfer_id;
	uint8_t ttl;
	uint16_t seq_num;
	uint16_t fin;
	uint16_t padding;
};*/
int create_data_sock();
int new_data_conn(int sock_index);
bool isData(int sock_index);
void remove_data_conn(int sock_index);
bool data_recv_hook(int sock_index);
int getNextHopID(uint32_t destIP);

#endif
