#ifndef INIT_H_
#define INIT_H_

void init_process_payload(char*);
void init_response(int sock_index);
void get_myIP();
int create_router_sock();
int create_data_sock();
int get_router_index(int router_id);

#endif
