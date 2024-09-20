#ifndef _UTIL_H_
#define _UTIL_H_

// set the bit in the given register
#define SET_BIT(REG, BIT) ((REG) |= (BIT))

// clear the bit in the given register
#define CLEAR_BIT(REG, BIT) ((REG) &= ~(BIT))

// get the vit in the given register
#define GET_BIT(REG, BIT) ((REG) & (BIT))

#define BIT0  0x001
#define BIT1  0x002
#define BIT2  0x004
#define BIT3  0x008
#define BIT4  0x010
#define BIT5  0x020
#define BIT6  0x040
#define BIT7  0x080
#define BIT8  0x100
#define BIT9  0x200
#define BIT10 0x400
#define BIT11 0x800

// get flag for the Nth bit (starting from bit 0)
#define BIT(N) (1 << (N))

#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_RED_BG  "\x1b[101m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_VIOLET  "\x1b[1;35m"
#define ANSI_CYAN    "\x1b[1;36m"
#define ANSI_RESET   "\x1b[0m"
#define DEBUG_STR    ANSI_RESET "[" ANSI_BLUE "DEBUG" ANSI_RESET "]"
#define ERROR_STR    ANSI_RESET "[" ANSI_RED "ERROR" ANSI_RESET "]"

#endif
