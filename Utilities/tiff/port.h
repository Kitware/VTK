/*
 * Warning, this file was automatically created by the TIFF configure script
 * VERSION:      v3.5.7
 * RELEASE:   
 * DATE:         Mon Dec 17 13:18:52 EST 2001
 * TARGET:       i686-pc-linux-gnu
 * CCOMPILER:    /usr/bin/gcc3-3.0.1-3)
 */
#ifndef _PORT_
#define _PORT_ 1
#ifdef __cplusplus
extern "C" {
#endif
#include <sys/types.h>
#define HOST_FILLORDER FILLORDER_MSB2LSB
#ifdef VTK_WORDS_BIGENDIAN
#define HOST_BIGENDIAN 1
#else
#define HOST_BIGENDIAN 0
#endif
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
typedef double dblparam_t;
#ifdef __STRICT_ANSI__
#define INLINE  __inline__
#else
#define INLINE  inline
#endif
#define GLOBALDATA(TYPE,NAME)   extern TYPE NAME
#ifdef __cplusplus
}
#endif
#endif
