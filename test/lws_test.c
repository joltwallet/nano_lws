#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include <libwebsockets.h>
#include "../include/nano_lws.h"



int get_block_count(){
    /* Works; returns the integer processed block count */
    int count_int;
    unsigned char rpc_command[1024];
    char rx_string[1024];
    
    snprintf( (char *) rpc_command, 1024, "{\"action\":\"block_count\"}" );
    printf("%s\n", rpc_command);
    network_get_data(rpc_command, rx_string);

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

int main()
{
    // printf() displays the string inside quotation
    printf("nano_lws lib: Check Block Count\n");
    
    
    while (1) {
        int actual_count = get_block_count();
        
        printf("%d\n", actual_count);
        
    }
    return 0;
}


