/* nanolws - libwebsocket wrapper
Copyright (C) 2018  Brian Pugh, James Coxon, Michael Smaili
https://www.joltwallet.com/

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef __INCLUDE_WEBSOCKET_H__
#define __INCLUDE_WEBSOCKET_H__

#define RX_BUFFER_BYTES (1536)
#define RECEIVE_POLLING_PERIOD_MS pdMS_TO_TICKS(10000)

int network_get_data(unsigned char *user_rpc_command, 
        unsigned char *result_data_buf, size_t result_data_buf_len);
void network_task(void *pvParameters);

#endif
