#ifndef __INCLUDE_WEBSOCKET_H__
#define __INCLUDE_WEBSOCKET_H__

#define RX_BUFFER_BYTES (1536)
#define RECEIVE_POLLING_PERIOD_MS pdMS_TO_TICKS(10000)

int get_data_via_ws(unsigned char *user_rpc_command, unsigned char *result_data);

#endif
