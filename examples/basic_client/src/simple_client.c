/*
dear coding club readers,
    - this is kind of broken a little bit, but it gives you some bones to work with
*/

#include <stdio.h>
#include <string.h>
#include "../../../src/cc_socket_api.h"

void handle_message(const char* data, int len) {
    printf("Received message: %.*s\n", len, data);
}

int main() {
    printf("Initializing socket API...\n");
    
    if (init_socket_api(handle_message) < 0) {
        printf("Failed to initialize socket API\n");
        return 1;
    }

    printf("Socket API initialized\n");
    printf("Commands:\n");
    printf("  connect <ip> - Connect to another instance\n");
    printf("  send <message> - Send a message to all connected clients\n");
    printf("  quit - Exit the program\n\n");

    char input[1024];
    char command[32];
    char arg[992];

    while (1) {
        printf("> ");
        fgets(input, sizeof(input), stdin);

        // parse cmd & arg
        arg[0] = '\0';
        if (sscanf(input, "%s %[^\n]", command, arg) < 1) {
            continue;
        }

        if (strcmp(command, "quit") == 0) {
            break;
        }
        else if (strcmp(command, "connect") == 0) {
            if (arg[0] == '\0') {
                printf("Usage: connect <ip>\n");
                continue;
            }
            if (connect_to(arg) < 0) {
                printf("cailed to connect to %s\n", arg);
            } else {
                printf("connected to %s\n", arg);
            }
        }
        else if (strcmp(command, "send") == 0) {
            if (arg[0] == '\0') {
                printf("Usage: send <message>\n");
                continue;
            }
            broadcast(arg, strlen(arg));
            printf("message broadcasted\n");
        }
        else {
            printf("bad command: %s\n", command);
        }
    }

    cleanup_socket_api();
    printf("bye\n");
    return 0;
}
