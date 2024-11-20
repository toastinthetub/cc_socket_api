#ifndef CC_SOCKET_API_H
#define CC_SOCKET_API_H

int init_socket_api(void (*on_receive)(const char* data, int len));

int connect_to(const char* ip_address);

void broadcast(const char* data, int len);

void cleanup_socket_api(void);

#endif 

