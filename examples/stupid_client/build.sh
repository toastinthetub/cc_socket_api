touch ./bin/stupid_client
gcc ./src/stupid_client.c ../../src/cc_socket_api.c -o ./bin/stupid_client -pthread -lncurses
 
