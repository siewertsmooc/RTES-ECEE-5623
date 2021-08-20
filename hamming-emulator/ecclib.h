#include <stdio.h>
#include <stdlib.h>

//#define MEM_SIZE (1024*1024)
#define MEM_SIZE (1024)
#define NO_ERROR (0)
#define PW_ERROR (-1)
#define SINGLE_BIT_ERROR_CORRECTED (-2)
#define DOUBLE_BIT_ERROR (-3)
#define UNKNOWN_ERROR (-4)

#define P01_BIT      (0x01)
#define P02_BIT      (0x02)
#define P03_BIT      (0x04)
#define P04_BIT      (0x08)
#define PW_BIT       (0x10)
#define ENCODED_BIT  (0x20)
#define UNUSED_BIT_7 (0x40)
#define UNUSED_BIT_8 (0x80)

#define DATA_BIT_1 (0x01)
#define DATA_BIT_2 (0x02)
#define DATA_BIT_3 (0x04)
#define DATA_BIT_4 (0x08)
#define DATA_BIT_5 (0x10)
#define DATA_BIT_6 (0x20)
#define DATA_BIT_7 (0x40)
#define DATA_BIT_8 (0x80)

#define SYNBITS    (0x0F)

typedef struct emulated_ecc
{
    unsigned char data_memory[MEM_SIZE];
    unsigned char code_memory[MEM_SIZE];
} ecc_t;

void print_code(unsigned char codeword);
void print_code_word(ecc_t *ecc, unsigned char *address);
void print_data_word(ecc_t *ecc, unsigned char *address);
void print_encoded(ecc_t *ecc, unsigned char *address);

unsigned char get_codeword(ecc_t *ecc, unsigned int offset);

int read_byte(ecc_t *ecc, unsigned char *address, unsigned char *byteRead);

int write_byte(ecc_t *ecc, unsigned char *address, unsigned char byteToWrite);

unsigned char *enable_ecc_memory(ecc_t *ecc);

void traceOn(void);
void traceOff(void);
