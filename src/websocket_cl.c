/*
 Functions your library should have:
 
 // Assumes wifi is already up and running
 setup_ws(params);
 
 // Populate work field given account field
 get_work(&block);
 
 //completely fill block fields with head block data
 get_head(&block);
 
 // Parse it into a process RPC command
 process_block(&block);
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <libwebsockets.h>
#include "../include/websocket_cl.h"

static struct lws *web_socket = NULL;
static struct lws_context *context = NULL;

char rx_string[RX_BUFFER_BYTES];
unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + RX_BUFFER_BYTES + LWS_SEND_BUFFER_POST_PADDING];
unsigned char *rpc_command = &buf[LWS_SEND_BUFFER_PRE_PADDING];
bool receive_complete = false;
bool command_sent = false;

void sleep_function(int milliseconds) {
    
    int seconds;
    
    if (milliseconds < 1000) {
        seconds = 1;
    }
    else {
        seconds = milliseconds / 1000;
    }
    
    sleep(seconds);
    
    //vTaskDelay(milliseconds / portTICK_PERIOD_MS);
    
}

static int ws_callback( struct lws *wsi, enum lws_callback_reasons reason,\
        void *user, void *in, size_t len ){
    switch( reason ){
        case LWS_CALLBACK_CLIENT_ESTABLISHED:{
            printf("Callback: Established\n");
            lws_callback_on_writable( wsi );
            break;
        }
        case LWS_CALLBACK_CLIENT_RECEIVE:{
            printf("Callback: Receive\n");
            ((char *) in)[len] = '\0';
            fprintf(stderr, "rx %d '%s'\n", (int)len, (char *)in);
            snprintf(rx_string, 1024, "%s", (char *) in);
            receive_complete = true;
            return 0;
            break;
        }
            
        case LWS_CALLBACK_CLIENT_WRITEABLE:{
            if(command_sent == false){
                printf("Callback: Writeable\n");
                size_t n = strlen((char *) rpc_command);
                printf("Command being sent out: %s\n", rpc_command); // debug print rpc command to terminal
                lws_write( web_socket, rpc_command, n, LWS_WRITE_TEXT );
                command_sent=true;
            }
            break;
        }
            
        case LWS_CALLBACK_CLOSED:{
            printf("Callback: Closed\n");
            web_socket = NULL;
            return 0;
            break;
        }
            
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:{
            printf("Callback: Error\n");
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

int network_get_data(unsigned char *user_rpc_command, unsigned char *result_data){
    
    if( !context){
        printf("Setting up lws\n");
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
        
        context = lws_create_context( &info );
        
        lws_set_log_level(0, lwsl_emit_syslog);
    }
    else {
        printf("Already setup\n");
    }
    
    if( !web_socket){
        
        printf("Opening ws\n");
        struct lws_client_connect_info ccinfo = {0};
        ccinfo.context = context;
        ccinfo.address = "yapraiwallet.space";
        ccinfo.path = "/";
        ccinfo.port = 8000;
        ccinfo.ssl_connection = 1;
        ccinfo.host = lws_canonical_hostname( context );
        ccinfo.origin = "origin";
        ccinfo.protocol = protocols[PROTOCOL_RAICAST].name;
        web_socket = lws_client_connect_via_info(&ccinfo);
        lws_service( context, /* timeout_ms = */ 0 );
    }
    
    printf("Now send the command\n");
    strcpy(rpc_command, user_rpc_command);
    receive_complete = false;
    command_sent = false;

    // Wait for received message
    while(!receive_complete){
        //printf("Waiting for callback, receive_complete = %d, command_sent = %d\n", receive_complete, command_sent);
        if (!web_socket) {
            break;
        }
        if (!command_sent){
            printf("Make callback request\n");
            lws_callback_on_writable( web_socket );
        }
        lws_service( context, 0 );
        sleep_function(300);
    }

    lws_service( context, /* timeout_ms = */ 0 );
    //lws_context_destroy(context);

    strcpy(result_data, rx_string);
    
    return 0;
}
