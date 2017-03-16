#ifndef __SOCKET_H__
#define __SOCKET_H__

#ifdef __cplusplus
extern "C" {
#endif

int SocketClientOpen(char *server_address, char *client_address, int port);
int SocketClientClose(void);
int SocketSend(char *buf, size_t buf_size);
int SocketRecv(char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif

