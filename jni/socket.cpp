#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sensor.h"
#include "socket.h"

#define RECV_BUF_SIZE (1024 * 10)
#define SOCKET_BUF_SIZE RECV_BUF_SIZE
static int gSock = -1;

int SocketClientOpen(char *server_address, char *client_address, int port)
{
	int *p_int = NULL;
	if (!server_address || !client_address || gSock > 0)
		return -1;
	
	gSock = socket(AF_INET, SOCK_STREAM, 0);
	if (gSock == -1) {
		LOGE("socket init error\n");
		return -1;
	}

	p_int = (int *)malloc(sizeof(int));
	*p_int = 1;
	if ((setsockopt(gSock, SOL_SOCKET, SO_REUSEADDR, &p_int, sizeof(int)) == -1 )
			|| (setsockopt(gSock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ) {
		LOGE("Error setting options \n");
		free(p_int);
	}
	free(p_int);

	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family = PF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = inet_addr(client_address);

	if (bind(gSock, (struct sockaddr*)&saddr, sizeof(saddr)) == -1) {
		LOGE("bind error\n");
		close(gSock);
		gSock = -1;
		return -1;
	}

	saddr.sin_addr.s_addr = inet_addr(server_address);

	if (connect(gSock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in)) == -1) {
		LOGE("connect error\n");
		close(gSock);
		gSock = -1;
		return -1;
	}

	LOGD("socket %d, server %s connet success(client = %s)\n", gSock, server_address, client_address);
	
	return 0;
}

int SocketSend(char *buf, size_t buf_size)
{
	if (gSock < 0)
		return -1;

	return send(gSock, buf, buf_size, MSG_DONTWAIT);
}

int SocketRecv(char *buf, size_t buf_size)
{
	if (gSock < 0)
		return -1;
	
	return recv(gSock, buf, buf_size, MSG_DONTWAIT);
}

int SocketClientClose(void)
{
	int res;

	if (gSock < 0)
		return -1;
	
	res = close(gSock);
	gSock = -1;
	
	return res;
}

