rm lws_test
gcc cJSON.c ../src/nano_lws.c lws_test.c -o lws_test -lwebsockets -lmbedtls -lmbedx509 -lmbedcrypto -lsodium
