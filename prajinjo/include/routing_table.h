#ifndef ROUTING_TABLE_H_
#define ROUTING_TABLE_H_

char* create_routing_table_payload(int payload_len);
void routing_table_response(int sock_index);
void send_dv();
void parse_dv(char *payload);
void run_bellman_ford();
char* create_dv_payload(int payload_len);

#endif
