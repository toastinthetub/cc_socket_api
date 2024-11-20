/* 
this program is possibly the simplest possible usage of our API:
    - listen for any data, and print anything we get. 
*/
#include <stdio.h>
#include <string.h>
#include "../../../src/cc_socket_api.h"

void handle_message(const char* data, int len) {
    printf("RECEIVED BYTES: %.*s\n", len, data);
}

int main() {
    printf("init socket API...\n");

    if (init_socket_api(handle_message) < 0) {
        printf("failed to initialize socket API\n");
        return 1;
    }

    printf("socket API initialized. listening for messages and printing them.\n");
    printf("press Ctrl+C to exit.\n");

    // we keep the program alive, as it is listening!
    while (1) {
      // don't do anything, our message handler is called asynchronously
    }

    cleanup_socket_api();
    return 0;
}

