#include <libwebsockets.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include "src/websocket_cl.h"

int main()
{
    // printf() displays the string inside quotation
    printf("Hello, World!\n");
    
    int count = 5;
    
    while (1) {
        int actual_count = get_block_count();
        count++;
        printf("Count = %d and %d\n", count, actual_count);
        sleep(count);
        
    }
    return 0;
}


