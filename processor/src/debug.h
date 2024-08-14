#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DEBUG_CPU   0x01
#define DEBUG_DRAM  0x02
#define DEBUG_FLAGS 0x04
#define DEBUG_ERRS  0x80

// mask used to enable/disable debugging
#define DEBUG (DEBUG_ERRS | DEBUG_CPU)

#endif
