#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DEBUG

#include "Arduino.h"

#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
    #define theSERIAL SerialUSB
#else
    #define theSERIAL Serial
#endif

#ifdef DEBUG
#define DMSG(args...)       theSERIAL.print(args)
#define DMSG_STR(str)       theSERIAL.println(str)
#define DMSG_HEX(num)       theSERIAL.print(' '); theSERIAL.print((num>>4)&0x0F, HEX); theSERIAL.print(num&0x0F, HEX)
#define DMSG_INT(num)       theSERIAL.print(' '); theSERIAL.print(num)
#else
#define DMSG(args...)
#define DMSG_STR(str)
#define DMSG_HEX(num)
#define DMSG_INT(num)
#endif

#endif
