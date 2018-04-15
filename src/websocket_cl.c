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

#include "../include/websocket_cl.h"
#include "../include/nano_lib.h"
#include "../include/helpers.h"
#include <libwebsockets.h>
#include <sodium.h>

#define BLOCK_BUFFER_SIZE 512

//const char RECEIVE_MIN[] = "1000000000000000000000000";
const char RECEIVE_MIN[] = "0";

static struct lws *web_socket = NULL;
static struct lws_context *context = NULL;

char block[BLOCK_BUFFER_SIZE];
char rx_string[RX_BUFFER_BYTES];

// Commands to send to server
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

void strupper(char *s){
    /* Converts a null-terminated string to uppercase */
    for(unsigned int c=0; s[c]!='\0'; c++){
        if (s[c] >= 'a' && s[c] <= 'z')
            s[c] = s[c] - 32;
    }
}

void strlower(char *s){
    /* Converts a null-terminated string to lowercase */
    for(unsigned int c=0; s[c]!='\0'; c++){
        if (s[c] >= 'A' && s[c] <= 'Z')
            s[c] = s[c] + 32;
    }
}

nl_err_t nl_public_to_address(char address_buf[], const uint8_t address_buf_len,
                              const uint256_t public_key){
    /* Translates a 256-bit binary public key into a NANO/XRB Address.
     *
     * address_buf will contain the resulting null terminated string
     *
     * This function does not contain sensitive data
     *
     * Based on Roosmaa's Ledger S Nano Github
     */
    uint8_t i, c;
    uint8_t check[CHECKSUM_LEN];
    #define CONFIG_NANO_LIB_ADDRESS_PREFIX "XRB_"
    
    crypto_generichash_state state;
    
    // sizeof includes the null character required
    if (address_buf_len < (sizeof(CONFIG_NANO_LIB_ADDRESS_PREFIX) + ADDRESS_DATA_LEN)){
        return E_INSUFFICIENT_BUF;
    }
    
    // Compute the checksum
    crypto_generichash_init( &state, NULL, 0, CHECKSUM_LEN);
    crypto_generichash_update( &state, public_key, BIN_256);
    crypto_generichash_final( &state, check, sizeof(check));
    
    // Copy in the prefix and shift pointer
    strlcpy(address_buf, CONFIG_NANO_LIB_ADDRESS_PREFIX, address_buf_len);
    address_buf += strlen(CONFIG_NANO_LIB_ADDRESS_PREFIX);
    
    // Helper macro to create a virtual array of check and public_key variables
#define accGetByte(x) (uint8_t)( \
((x) < 5) ? check[(x)] : \
((x) - 5 < 32) ? public_key[32 - 1 - ((x) - 5)] : \
0 \
)
    for (int k = 0; k < ADDRESS_DATA_LEN; k++) {
        i = (k / 8) * 5;
        c = 0;
        switch (k % 8) {
            case 0:
                c = accGetByte(i) & B_11111;
                break;
            case 1:
                c = (accGetByte(i) >> 5) & B_00111;
                c |= (accGetByte(i + 1) & B_00011) << 3;
                break;
            case 2:
                c = (accGetByte(i + 1) >> 2) & B_11111;
                break;
            case 3:
                c = (accGetByte(i + 1) >> 7) & B_00001;
                c |= (accGetByte(i + 2) & B_01111) << 1;
                break;
            case 4:
                c = (accGetByte(i + 2) >> 4) & B_01111;
                c |= (accGetByte(i + 3) & B_00001) << 4;
                break;
            case 5:
                c = (accGetByte(i + 3) >> 1) & B_11111;
                break;
            case 6:
                c = (accGetByte(i + 3) >> 6) & B_00011;
                c |= (accGetByte(i + 4) & B_00111) << 2;
                break;
            case 7:
                c = (accGetByte(i + 4) >> 3) & B_11111;
                break;
        }
        address_buf[ADDRESS_DATA_LEN-1-k] = BASE32_ALPHABET[c];
    }
#undef accGetByte
    
    address_buf[ADDRESS_DATA_LEN] = '\0';
    return E_SUCCESS;
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
            sprintf(rx_string, "%s", (char *) in);
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

int get_data_via_ws(){
    
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
        //ccinfo.address = "light.nano.org";
        ccinfo.address = "yapraiwallet.space";
        ccinfo.path = "/";
        //ccinfo.port = 443;
        ccinfo.port = 8000;
        ccinfo.ssl_connection = 1;
        ccinfo.host = lws_canonical_hostname( context );
        ccinfo.origin = "origin";
        ccinfo.protocol = protocols[PROTOCOL_RAICAST].name;
        web_socket = lws_client_connect_via_info(&ccinfo);
        lws_service( context, /* timeout_ms = */ 0 );
    }
    
    printf("Now send the command\n");
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
    return 0;
}

int get_block_count(){
    /* Works; returns the integer processed block count */
    int count_int;
    
    sprintf( (char *) rpc_command, "{\"action\":\"block_count\"}" );
    get_data_via_ws();
    
    const cJSON *count = NULL;
    cJSON *json = cJSON_Parse(rx_string);
    
    count = cJSON_GetObjectItemCaseSensitive(json, "count");
    if (cJSON_IsString(count) && (count->valuestring != NULL))
    {
        count_int = atoi(count->valuestring);
    }
    else{
        count_int = 0;
    }
    
    cJSON_Delete(json);
    
    return count_int;
}

int get_frontier(char *account_address, char *frontier_block_hash){

    int outcome;
    
    strlower(account_address);
    
    sprintf( (char *) rpc_command,
            "{\"action\":\"accounts_frontiers\",\"accounts\":[\"%s\"]}",
            account_address);
    get_data_via_ws();
    
    const cJSON *frontiers = NULL;
    const cJSON *account = NULL;
    
    cJSON *json = cJSON_Parse(rx_string);
    
    frontiers = cJSON_GetObjectItemCaseSensitive(json, "frontiers");
    
    account = cJSON_GetObjectItemCaseSensitive(frontiers, account_address);
    
    if (cJSON_IsString(account) && (account->valuestring != NULL))
    {
        strcpy(frontier_block_hash, account->valuestring);
        outcome = 1;
    }
    else{
        outcome = 0;
    }
    
    cJSON_Delete(json);
    
    return outcome;
}

char* deblank(char* input)
{
    int i,j;
    char *output=input;
    for (i = 0, j = 0; i<strlen(input); i++,j++)
    {
        if (input[i]!=' ')
            output[j]=input[i];
        else
            j--;
    }
    output[j]=0;
    return output;
}

char* replace(char* str, char* a, char* b)
{
    int len  = strlen(str);
    int lena = strlen(a), lenb = strlen(b);
    for (char* p = str; p = strstr(p, a); ++p) {
        if (lena != lenb) // shift end as needed
            memmove(p+lenb, p+lena,
                    len - (p - str) + lenb);
        memcpy(p, b, lenb);
    }
    return str;
}

int get_head(nl_block_t *block){
    
    char account_address[ADDRESS_BUF_LEN];
    
    //Now convert this to an XRB address
    nl_err_t res;
    res = nl_public_to_address(account_address,
                               sizeof(account_address),
                               block->account);
    strupper(account_address);
    printf("Address: %s\n", account_address);
    
    //Get latest block from server
    //First get frontier
    char frontier_block_hash[65];
    
    //get_frontier(account_address, frontier_block_hash);
    strcpy(frontier_block_hash, "54E3CDEEDF790136FF8FD47105D1008F46BA42A1EC7790A1B43E1AC381EDFA80");
    printf("Frontier Block: %s\n", frontier_block_hash);
    
    //Now get the block info
    sprintf( (char *) rpc_command,
            "{\"action\":\"block\",\"hash\":\"%s\"}",
            frontier_block_hash);

    get_data_via_ws();
 
    block->type = STATE;
    
    const cJSON *json_contents = NULL;
    const cJSON *json_previous = NULL;
    const cJSON *json_link = NULL;
    const cJSON *json_representative = NULL;
    const cJSON *json_account = NULL;
    const cJSON *json_balance = NULL;
    const cJSON *json_work = NULL;
    const cJSON *json_signature = NULL;
    
    cJSON *json = cJSON_Parse(rx_string);
    
    json_contents = cJSON_GetObjectItemCaseSensitive(json, "contents");
    char *string = cJSON_Print(json_contents);

    char* new_string = replace(string, "\\n", "\\");
    
    for (char* p = new_string; (p = strchr(p, '\\')); ++p) {
        *p = ' ';
    }
    
    char * new_string_nws = deblank(new_string);
    
    new_string_nws[0] = ' ';
    new_string_nws[strlen(new_string_nws)-1] = ' ';
    
    cJSON *nested_json = cJSON_Parse(new_string_nws);
    
    json_account = cJSON_GetObjectItemCaseSensitive(nested_json, "account");
    
    if (cJSON_IsString(json_account) && (json_account->valuestring != NULL))
    {
        printf("Account: %s\n", json_account->valuestring);
        //TODO We need to convert this to a public key
        sodium_hex2bin(block->account, sizeof(block->account),
                       json_account->valuestring,
                       HEX_256, NULL, NULL, NULL);
    }
    
    json_previous = cJSON_GetObjectItemCaseSensitive(nested_json, "previous");
    if (cJSON_IsString(json_previous) && (json_previous->valuestring != NULL))
    {
        
        sodium_hex2bin(block->previous, sizeof(block->previous),
                       json_previous->valuestring,
                       HEX_256, NULL, NULL, NULL);
    }
    
    json_representative = cJSON_GetObjectItemCaseSensitive(nested_json, "representative");
    if (cJSON_IsString(json_representative) && (json_representative->valuestring != NULL))
    {
        printf("Representative: %s\n", json_representative->valuestring);
        //TODO We need to convert this to a public key
        sodium_hex2bin(block->representative, sizeof(block->representative),
                       json_representative->valuestring,
                       HEX_256, NULL, NULL, NULL);
    }

    json_signature = cJSON_GetObjectItemCaseSensitive(nested_json, "signature");
    if (cJSON_IsString(json_signature) && (json_signature->valuestring != NULL))
    {
        sodium_hex2bin(block->signature, sizeof(block->signature),
                       json_signature->valuestring,
                       HEX_512, NULL, NULL, NULL);
    }
    
    json_link = cJSON_GetObjectItemCaseSensitive(nested_json, "link");
    if (cJSON_IsString(json_link) && (json_link->valuestring != NULL))
    {
        sodium_hex2bin(block->link, sizeof(block->link),
                       json_link->valuestring,
                       HEX_256, NULL, NULL, NULL);
    }
    
    json_work = cJSON_GetObjectItemCaseSensitive(nested_json, "work");
    if (cJSON_IsString(json_work) && (json_work->valuestring != NULL))
    {
        block->work = json_work->valuestring;
    }
  
    json_balance = cJSON_GetObjectItemCaseSensitive(nested_json, "balance");
    if (cJSON_IsString(json_balance) && (json_balance->valuestring != NULL))
    {
        printf("Balance: %s\n", json_balance->valuestring);
        const mbedtls_mpi current_balance;
        mbedtls_mpi_init(&current_balance);
        mbedtls_mpi_read_string(&current_balance, 10, json_balance->valuestring);
        mbedtls_mpi_copy (&(block->balance), &current_balance);
        
        mbedtls_mpi_free (&current_balance);
        
    }
    
    cJSON_Delete(json);


    return 0;
}

int process_block(nl_block_t *block){

    //Account (convert bin to address)
    char account_address[ADDRESS_BUF_LEN];
    nl_err_t res;
    res = nl_public_to_address(account_address,
                               sizeof(account_address),
                               block->account);
    strlower(account_address);
    printf("Address: %s\n", account_address);
    
    //Previous (convert bin to hex)
    hex512_t previous_hex;
    sodium_bin2hex(previous_hex, sizeof(previous_hex),
                   block->previous, sizeof(block->previous));
    printf("Previous: %s\n", previous_hex);
    
    //Representative (convert bin to address)
    char representative_address[ADDRESS_BUF_LEN];
    res = nl_public_to_address(representative_address,
                               sizeof(representative_address),
                               block->representative);
    strlower(representative_address);
    printf("Representative: %s\n", representative_address);
    
    //Balance (convert mpi to string)
    static char balance_buf[64];
    size_t n;
    memset(balance_buf, 0, sizeof(balance_buf));
    mbedtls_mpi_write_string(&block->balance, 10, balance_buf, sizeof(balance_buf)-1, &n);
    printf("Balance: %s\n", balance_buf);
    
    //Link (convert bin to hex)
    hex256_t link_hex;
    sodium_bin2hex(link_hex, sizeof(link_hex),
                   block->link, sizeof(block->link));
    printf("Link: %s\n", link_hex);
    
    //Work (keep as hex)
    printf("Work: %llu\n", block->work);
    
    //Signature (convert bin to hex)
    hex512_t signature_hex;
    sodium_bin2hex(signature_hex, sizeof(signature_hex),
                   block->signature, sizeof(block->signature));
    printf("Block->signature: %s\n", signature_hex);
    
    sprintf( block,
            "{"
            "\\\"type\\\":\\\"state\\\","
            "\\\"account\\\":\\\"%s\\\","
            "\\\"previous\\\":\\\"%s\\\","
            "\\\"representative\\\":\\\"%s\\\","
            "\\\"balance\\\":\\\"%s\\\","
            "\\\"link\\\":\\\"%s\\\","
            "\\\"work\\\":\\\"%s\\\","
            "\\\"signature\\\":\\\"%s\\\""
            "}",
            account_address, previous_hex, representative_address, balance_buf, link_hex, block->work, signature_hex
            );
    
    printf("\nBlock:\n%s\n", block);
    
    sprintf( (char *) rpc_command,
            "{"
            "\"action\":\"process\","
            "\"block\":\"%s\""
            "}",
            block);
    get_data_via_ws();
    
    return 0;
}
