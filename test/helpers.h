#ifndef __NANO_LIB_HELPERS_H__
#define __NANO_LIB_HELPERS_H__

#include "byteswap.h"

#ifndef bswap_64
#define bswap_64(x) __bswap_64(x)
#endif

#ifndef bswap_32
#define bswap_32(x) __bswap_32(x)
#endif

#define CHECKSUM_LEN 5
#define ADDRESS_DATA_LEN 60 // Does NOT include null character
#define B_11111 31
#define B_01111 15
#define B_00111  7
#define B_00011  3
#define B_00001  1

void strupper(char *s);
void strnupper(char *s, const int n);
void strlower(char *s);
void strnlower(char *s, const int n);

void nl_generate_seed(uint256_t seed_bin);

#endif
