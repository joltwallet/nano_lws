rm lws_test
gcc src/cJSON.c src/websocket_cl.c lws_test.c -o lws_test -lwebsockets -lmbedtls -lmbedx509 -lmbedcrypto -lsodium -g -fstack-protector
