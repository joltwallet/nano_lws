#ifndef __INCLUDE_WEBSOCKET_H__
#define __INCLUDE_WEBSOCKET_H__

#define RX_BUFFER_BYTES (1024)
#define RECEIVE_POLLING_PERIOD_MS pdMS_TO_TICKS(10000)

// todo: maybe do something fancier with malloc
#define RAW_DECIMAL_SIZE 39 + 1 + 8 // because of issues with mbedtls_mpi
#define RAW_HEX_SIZE 32 + 1

#define BIN_128 16
#define BIN_256 32
#define BIN_512 64

// Plus one since these will always be used for ascii values
#define HEX_128 2*BIN_128 + 1
#define HEX_256 2*BIN_256 + 1
#define HEX_512 2*BIN_512 + 1

#define BLOCK_BUFFER_SIZE 512

//#include "mbedtls/bignum.h"
#include "cjson/cJSON.h"

int get_block_count();

#endif
