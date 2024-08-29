#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DEBUG_CPU   0x01
#define DEBUG_DRAM  0x02
#define DEBUG_FLAGS 0x04
#define DEBUG_ERRS  0x80

// mask used to enable/disable debugging
#define DEBUG (DEBUG_ERRS | DEBUG_CPU | DEBUG_DRAM)

#if DEBUG & DEBUG_CPU
    #define DEBUG_CPU_PRINT(...) printf(__VA_ARGS__);
#else
    #define DEBUG_CPU_PRINT(...)
#endif

#if DEBUG & DEBUG_FLAGS
    #define DEBUG_FLAGS_PRINT(...) printf(__VA_ARGS__);
#else
    #define DEBUG_FLAGS_PRINT(...)
#endif

#if DEBUG & DEBUG_ERRS
    #define ERR_PRINT(...) printf(ERROR_STR " " __VA_ARGS__);
#else
    #define ERR_PRINT(...)
#endif

#endif
