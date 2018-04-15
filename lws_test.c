#include <libwebsockets.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
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
        mbedtls_mpi_write_string(&block.balance, 10, buf, sizeof(buf)-1, &n);
        
        printf("Block->balance: %s\n", buf);
        
        //Now send your own block
        printf("\nPrinting new block\n");
        process_block(&block);
        
        loop_count = 10;
        
        printf("Count = %d and %d and %d\n", loop_count, actual_count, outcome);
        sleep(loop_count);
        
    }
    return 0;
}


