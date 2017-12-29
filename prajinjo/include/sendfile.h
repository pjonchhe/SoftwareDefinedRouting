#ifndef SENDFILE_H_
#define SENDFILE_H_

int process_sendfile_payload(char* payload, int len);
int connect_to_next_hop(int id);
void send_response(int sock_index);
char* create_datapkt_header(uint32_t destIP, uint8_t transfer_id, uint8_t ttl, uint16_t seq_num, bool fin);
void transfer_file(int socket, uint32_t destIP, uint8_t transfer_id, uint8_t ttl, uint16_t seq_num, char* fileName);
char *update_datapkt_header(char* datapkt_header, uint16_t seq_num, bool fin);
void sendfilestats_response(int sock_index, char *payload);
void lastdatapacket_response(int sock_index);
void penultimate_response(int sock_index);

#endif
