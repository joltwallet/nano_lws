#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "include/nano_lib.h"
#include "include/helpers.h"
#include <sodium.h>
#include <libwebsockets.h>
#include "include/websocket_cl.h"

void nl_block_init(nl_block_t *block){
    block->type = UNDEFINED;
    sodium_memzero(block->account, sizeof(block->account));
    sodium_memzero(block->previous, sizeof(block->previous));
    sodium_memzero(block->representative, sizeof(block->representative));
    sodium_memzero(&(block->work), sizeof(block->work));
    sodium_memzero(block->signature, sizeof(block->signature));
    sodium_memzero(block->link, sizeof(block->link));
    mbedtls_mpi_init(&(block->balance));
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

int get_block_count(){
    /* Works; returns the integer processed block count */
    int count_int;
    unsigned char rpc_command[1024];
    char rx_string[1024];
    
    snprintf( (char *) rpc_command, 1024, "{\"action\":\"block_count\"}" );
    printf("%s\n", rpc_command);
    get_data_via_ws(rpc_command, rx_string);

    printf("Rx String: %s\n", rx_string);
    
    
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
/*

int get_frontier(char *account_address, char *frontier_block_hash){
    
    int outcome;
    
    strlower(account_address);
    
    snprintf( (char *) rpc_command, 1024,
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
    snprintf( (char *) rpc_command, 1024,
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
        mbedtls_mpi current_balance;
        printf("1\n");
        mbedtls_mpi_init(&current_balance);
        printf("2\n");
        mbedtls_mpi_read_string(&current_balance, 10, json_balance->valuestring);
        printf("3\n");
        mbedtls_mpi_copy( &(block->balance), &current_balance);
        printf("4\n");
        
        mbedtls_mpi_free (&current_balance);
        printf("5\n");
        
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
    char balance_buf[64];
    size_t n;
    memset(balance_buf, 0, sizeof(balance_buf));
    mbedtls_mpi_write_string(&(block->balance), 10, balance_buf, sizeof(balance_buf)-1, &n);
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
    
    int error = snprintf( block,1024,
                         "{"
                         "\\\"type\\\":\\\"state\\\","
                         "\\\"account\\\":\\\"%s\\\","
                         "\\\"previous\\\":\\\"%s\\\","
                         "\\\"representative\\\":\\\"%s\\\","
                         "\\\"balance\\\":\\\"%s\\\","
                         "\\\"link\\\":\\\"%s\\\","
                         "\\\"work\\\":\\\"%llu\\\","
                         "\\\"signature\\\":\\\"%s\\\""
                         "}",
                         //            account_address, previous_hex, representative_address, balance_buf, link_hex, block->work, signature_hex
                         "0", "0", "0", balance_buf, "0", block->work, signature_hex
                         );
    
    printf("\nBlock: %d\n%s\n", error, block);
    
    snprintf( (char *) rpc_command, 1024,
             "{"
             "\"action\":\"process\","
             "\"block\":\"%s\""
             "}",
             block);
    get_data_via_ws();
    
    
    return 0;
}

*/
int main()
{
    // printf() displays the string inside quotation
    printf("Hello, World!\n");
    
    int loop_count = 5;
    nl_block_t block;
    
    nl_block_init(&block);
    
    //Load up with an account, now we can populate the rest.
    sodium_hex2bin(block.account, sizeof(block.account),
                   "C1CD33D62CC72FAC1294C990D4DD2B02A4DB85D42F220C48C13AF288FB21D4C1",
                   HEX_256, NULL, NULL, NULL);
    
    
    while (1) {
        int actual_count = get_block_count();
        
        printf("%d\n", actual_count);
        /*
        
        int outcome = get_head(&block);
        printf("Block returned\n");
        
        printf("Block->type: %d\n", block.type);
        
        hex256_t account_hex;
        sodium_bin2hex(account_hex, sizeof(account_hex),
                       block.account, sizeof(block.account));
        printf("Block->account: %s\n", account_hex);
        
        hex256_t previous_hex;
        sodium_bin2hex(previous_hex, sizeof(previous_hex),
                       block.previous, sizeof(block.previous));
        printf("Block->previous: %s\n", previous_hex);
        
        hex256_t representative_hex;
        sodium_bin2hex(representative_hex, sizeof(representative_hex),
                       block.representative, sizeof(block.representative));
        printf("Block->representative: %s\n", representative_hex);

        hex512_t signature_hex;
        sodium_bin2hex(signature_hex, sizeof(signature_hex),
                       block.signature, sizeof(block.signature));
        printf("Block->signature: %s\n", signature_hex);
        
        hex256_t link_hex;
        sodium_bin2hex(link_hex, sizeof(link_hex),
                       block.link, sizeof(block.link));
        printf("Block->link: %s\n", link_hex);
        
        printf("Block->work: %llu\n", block.work);
        
        
        static char buf[64];
        size_t n;
        memset(buf, 0, sizeof(buf));
        printf("Print balance\n");
        mbedtls_mpi_write_string(&block.balance, 10, buf, sizeof(buf)-1, &n);
        
        printf("Block->balance: %s\n", buf);
        
        //Now send your own block
        printf("\nPrinting new block\n");
        process_block(&block);
        
        loop_count = 10;
        
        printf("Count = %d and %d and %d\n", loop_count, actual_count, outcome);
        sleep(loop_count);
         */
        
    }
    return 0;
}


