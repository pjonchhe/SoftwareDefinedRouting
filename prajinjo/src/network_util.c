#include <sys/time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include "../include/global.h"
ssize_t recvALL(int sock_index, char *buffer, ssize_t nbytes)
{
	ssize_t bytes = 0;
	bytes = recv(sock_index, buffer, nbytes, 0);
	if(bytes == 0) return -1;
	while(bytes != nbytes)
	{
		bytes += recv(sock_index, buffer+bytes, nbytes-bytes, 0);
	}

	return bytes;
}

ssize_t sendALL(int sock_index, char *buffer, ssize_t nbytes)
{
	ssize_t bytes = 0;
	struct timeval cur_temp1,cur_temp2, tv;
gettimeofday(&cur_temp1, NULL);
	bytes = send(sock_index, buffer, nbytes, 0);
	gettimeofday(&cur_temp2, NULL);

                        timersub (&cur_temp2, &cur_temp1, &tv);
                        printf("tv.sec = %ld tv.usec = %ld\n", tv.tv_sec, tv.tv_usec);
	
	if(bytes == 0) return -1;
	while(bytes != nbytes)
	{
		bytes += send(sock_index, buffer+bytes, nbytes-bytes, 0);
	}

	return bytes;
}

ssize_t sendtoALL(int sock_index, char *buffer, ssize_t nbytes, unsigned int flags, const struct sockaddr *to, socklen_t tolen)
{
	ssize_t bytes = 0;
	bytes = sendto(sock_index, buffer, nbytes, 0, to, tolen);
	FILE *test;
        char filename[100];
        strcpy(filename, MY_IP);
        strcat(filename, "dump");
        test = fopen(filename, "a");
        fwrite(buffer, nbytes, 1, test);
        fclose(test);

	if(bytes == 0) return -1;
	while(bytes != nbytes)
	{
		bytes += sendto(sock_index, buffer+bytes, nbytes-bytes, 0, to, tolen);
	}

	return bytes;
}

ssize_t recvfromALL(int sock_index, char *buffer, ssize_t nbytes, unsigned int flags, struct sockaddr *from, int *fromlen)
{
	ssize_t bytes = 0;
	bytes = recvfrom(sock_index, buffer, nbytes, 0, from, fromlen);
	if(bytes == 0) return -1;
	while(bytes != nbytes)
	{
		bytes += recvfrom(sock_index, buffer+bytes, nbytes-bytes, 0, from, fromlen);
	}

	FILE *test;
        char filename[100];
        strcpy(filename, MY_IP);
        strcat(filename, "recvdump");
        test = fopen(filename, "a");
        fwrite(buffer, nbytes, 1, test);
        fclose(test);


	return bytes;
}
