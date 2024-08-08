#ifndef _UTIL_H_
#define _UTIL_H_

// set the bit in the given register
#define SET_BIT(REG, BIT) ((REG) |= (BIT))

// clear the bit in the given register
#define CLEAR_BIT(REG, BIT) ((REG) &= ~(BIT))

// get the vit in the given register
#define GET_BIT(REG, BIT) ((REG) & (BIT))

#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80

// get flag for the Nth bit (starting from bit 0)
#define BIT(N) (1 << (N))

#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_RESET   "\x1b[0m"
#define DEBUG_STR    ANSI_RESET "[" ANSI_BLUE "DEBUG" ANSI_RESET "]"
#define ERROR_STR    ANSI_RESET "[" ANSI_RED "ERROR" ANSI_RESET "]"

#endif
