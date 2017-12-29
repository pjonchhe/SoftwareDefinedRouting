#ifndef CONNECTION_MANAGER_H_
#define CONNECTION_MANAGER_H_

int control_socket, router_socket, data_socket;
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);
void init();

#endif
