#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 42069
#define MAX_BUFFER 1024
#define MAX_CONNECTIONS 10

typedef struct {
    int socket;
    void (*on_receive)(const char* data, int len);
} Connection;

static Connection connections[MAX_CONNECTIONS];
static int connection_count = 0;
static pthread_mutex_t connection_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t listener_thread;
static int server_socket;

// handle data coming
void* connection_handler(void* connection_ptr) {
    Connection* conn = (Connection*)connection_ptr;
    char buffer[MAX_BUFFER];
    
    while (1) {
        int bytes_received = recv(conn->socket, buffer, MAX_BUFFER - 1, 0);
        if (bytes_received <= 0) {
            close(conn->socket);
            break;
        }
        
        buffer[bytes_received] = '\0';
        if (conn->on_receive) {
            conn->on_receive(buffer, bytes_received);
        }
    }
    
    return NULL;
}

// fn listen for conncetion
void* listener_function(void* arg) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    while (1) {
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) continue;
        
        pthread_mutex_lock(&connection_mutex);
        if (connection_count < MAX_CONNECTIONS) {
            connections[connection_count].socket = client_socket;
            
            pthread_t thread;
            pthread_create(&thread, NULL, connection_handler, &connections[connection_count]);
            pthread_detach(thread);
            
            connection_count++;
        } else {
            close(client_socket);
        }
        pthread_mutex_unlock(&connection_mutex);
    }
    return NULL;
}

// all that jazz
int init_socket_api(void (*on_receive)(const char* data, int len)) {
    // create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // i don't even know what port reuse really is
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // config server addy
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // bind the socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return -1;
    }
    
    // listen 4 knocks
    if (listen(server_socket, MAX_CONNECTIONS) < 0) {
        perror("Listen failed");
        return -1;
    }
    
    // connection callback
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        connections[i].on_receive = on_receive;
    }
    
    // c socket api suskc
    pthread_create(&listener_thread, NULL, listener_function, NULL);
    
    return 0;
}

// connect to another machine listening on 42069
int connect_to(const char* ip_address) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }
    
    pthread_mutex_lock(&connection_mutex);
    if (connection_count < MAX_CONNECTIONS) {
        connections[connection_count].socket = sock;
        
        pthread_t thread;
        pthread_create(&thread, NULL, connection_handler, &connections[connection_count]);
        pthread_detach(thread);
        
        connection_count++;
    } else {
        close(sock);
        sock = -1;
    }
    pthread_mutex_unlock(&connection_mutex);
    
    return sock;
}

// broadcast 2 all connected
void broadcast(const char* data, int len) {
    pthread_mutex_lock(&connection_mutex);
    for (int i = 0; i < connection_count; i++) {
        send(connections[i].socket, data, len, 0);
    }
    pthread_mutex_unlock(&connection_mutex);
}

// clean
void cleanup_socket_api() {
    pthread_mutex_lock(&connection_mutex);
    for (int i = 0; i < connection_count; i++) {
        close(connections[i].socket);
    }
    pthread_mutex_unlock(&connection_mutex);
    
    close(server_socket);
    pthread_cancel(listener_thread);
}
