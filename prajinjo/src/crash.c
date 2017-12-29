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

void crash_response(int sock_index)
{
	uint16_t response_len, payload_len = 0;
	char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;
	
	cntrl_response_header = create_response_header(sock_index, 4, 0, payload_len);
	response_len = CNTRL_RESP_HEADER_SIZE + payload_len;
	cntrl_response = (char *) malloc(response_len);
	
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);

	sendALL(sock_index, cntrl_response, response_len);
	free(cntrl_response);
}
