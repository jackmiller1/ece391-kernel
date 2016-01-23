/* types.h - Defines to use the familiar explicitly-sized types in this
 * OS (uint32_t, int8_t, etc.).  This is necessary because we don't want
 * to include <stdint.h> when building this OS
 * vim:ts=4 noexpandtab
 */

#ifndef _TYPES_H
#define _TYPES_H

#define NULL 0
#define _128MB 0x8000000
#define _132MB 0x8400000
#define _136MB 0x8800000
#define _100MB 0x6400000
#define _8MB 0x800000
#define _4MB 0x400000
#define _8KB 0x2000
#define _4KB 0x1000
#define ASCII_DEL 0x7f
#define ASCII_E 0x45
#define ASCII_L 0x4c
#define ASCII_F 0x46
#define ASCII_NL 0x0A


#ifndef ASM

/* Types defined here just like in <stdint.h> */
typedef int int32_t;
typedef unsigned int uint32_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef char int8_t;
typedef unsigned char uint8_t;

typedef struct{
    int8_t fileName[33];
    unsigned int fileType;
    unsigned int inodeNumber;
}dentry_t;

#endif /* ASM */

#endif /* _TYPES_H */
