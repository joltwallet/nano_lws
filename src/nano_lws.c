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

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <libwebsockets.h>
#include "esp_log.h"

#include "nano_lws.h"

static struct lws *web_socket = NULL;
static struct lws_context *context = NULL;

char rx_string[RX_BUFFER_BYTES];
unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + RX_BUFFER_BYTES + LWS_SEND_BUFFER_POST_PADDING];
unsigned char *rpc_command = &buf[LWS_SEND_BUFFER_PRE_PADDING];
bool receive_complete = false;
bool command_sent = false;

static const char *TAG = "network_task";

void sleep_function(int milliseconds) {
    vTaskDelay(milliseconds / portTICK_PERIOD_MS);
}

static int ws_callback( struct lws *wsi, enum lws_callback_reasons reason,\
        void *user, void *in, size_t len ){
    switch( reason ){
        case LWS_CALLBACK_CLIENT_ESTABLISHED:{
            ESP_LOGI(TAG, "Callback: Established\n");
            lws_callback_on_writable( wsi );
            break;
        }
        case LWS_CALLBACK_CLIENT_RECEIVE:{
            ESP_LOGI(TAG, "Callback: Receive\n");
            ((char *) in)[len] = '\0';
            //fprintf(stderr, "rx %d '%s'\n", (int)len, (char *)in);
            snprintf(rx_string, 1024, "%s", (char *) in);
            receive_complete = true;
            return 0;
            break;
        }
            
        case LWS_CALLBACK_CLIENT_WRITEABLE:{
            ESP_LOGI(TAG, "Callback: Writeable\n");
            if(command_sent == false){
                size_t n = strlen((char *) rpc_command);
                ESP_LOGI(TAG, "Command being sent out: %s\n", rpc_command); // debug print rpc command to terminal
                lws_write( web_socket, rpc_command, n, LWS_WRITE_TEXT );
                command_sent=true;
            }
            break;
        }
            
        case LWS_CALLBACK_CLOSED:{
            ESP_LOGI(TAG, "Callback: Closed\n");
            web_socket = NULL;
            return 0;
            break;
        }
            
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:{
            ESP_LOGI(TAG, "Callback: Error %d\n", LWS_CALLBACK_CLIENT_CONNECTION_ERROR);
            ESP_LOGI(TAG, "CLIENT_CONNECTION_ERROR: %s\n",
                     in ? (char *)in : "(null)");
            web_socket = NULL;
            return -1;
            break;
        }
            
        default:{
            break;
        }
    }
    
    return 0;
}

enum protocols{
    PROTOCOL_RAICAST = 0,
    PROTOCOL_COUNT
};

static struct lws_protocols protocols[] ={
    {
        "example-protocol",
        ws_callback,
        0,
        RX_BUFFER_BYTES,
    },
    { NULL, NULL, 0, 0 } /* terminator */
};

void network_task(void *pvParameters)
{
    
    while (1) {
        
        vTaskDelay(120000 / portTICK_PERIOD_MS);
        ESP_LOGV(TAG, "RESET Websocket");
        web_socket = NULL;
    }
    
    vTaskDelete( NULL );
}

int network_get_data(unsigned char *user_rpc_command,
        unsigned char *result_data_buf, size_t result_data_buf_len){
    
    if( !context){
        ESP_LOGI(TAG, "Setting up lws\n");
        struct lws_context_creation_info info;
        memset( &info, 0, sizeof(info) );
        
        info.port = CONTEXT_PORT_NO_LISTEN;
        info.protocols = protocols;
        info.gid = -1;
        info.uid = -1;
        info.max_http_header_pool = 1;
        info.max_http_header_data = 1024;
        info.pt_serv_buf_size = 1024;
        
        info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        //info.options |= LWS_SERVER_OPTION_JUST_USE_RAW_ORIGIN;
        
        context = lws_create_context( &info );
        
        lws_set_log_level(0, lwsl_emit_syslog);
    }
    else {
        ESP_LOGI(TAG, "Already setup\n");
    }
    
    if( !web_socket){
        
        ESP_LOGI(TAG, "Opening ws\n");
        struct lws_client_connect_info ccinfo = {0};
        memset( &ccinfo, 0, sizeof(ccinfo) );
        
        ccinfo.context = context;
        ccinfo.address = CONFIG_NANO_LWS_DOMAIN;
        ccinfo.path = CONFIG_NANO_LWS_PATH;
        ccinfo.port = CONFIG_NANO_LWS_PORT;
        ccinfo.ssl_connection = 0;
        ccinfo.host = lws_canonical_hostname( context );
        ccinfo.origin = "origin";
        ccinfo.protocol = protocols[PROTOCOL_RAICAST].name;
        web_socket = lws_client_connect_via_info(&ccinfo);
        lws_service( context, /* timeout_ms = */ 0 );
        sleep_function(100);
        
        ESP_LOGI(TAG, "%s\n", ccinfo.address);
    }
    
    ESP_LOGI(TAG, "Now send the command\n");
    strlcpy((char *)rpc_command, (char *)user_rpc_command, RX_BUFFER_BYTES);
    receive_complete = false;
    command_sent = false;

    // Wait for received message
    while(!receive_complete){
        //ESP_LOGI(TAG, "Waiting for callback, receive_complete = %d, command_sent = %d\n", receive_complete, command_sent);
        if (!web_socket) {
            ESP_LOGI(TAG, "No websocket, breaking\n");
            break;
        }
        if (!command_sent){
            ESP_LOGI(TAG, "Make callback request\n");
            lws_callback_on_writable( web_socket );
        }
        lws_service( context, 0 );
        sleep_function(100);
    }

    lws_service( context, /* timeout_ms = */ 0 );
    //lws_context_destroy(context);

    strlcpy((char *)result_data_buf, (char *)rx_string, result_data_buf_len);
    
    return 0;
}
