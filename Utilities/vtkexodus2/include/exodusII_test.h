#ifndef __exodusII_test_h
#define __exodusII_test_h

#include "exodusII.h"

/* Need these includes for vtkstd namespace and cout/cerr */
#include "vtkIOStream.h"
#include "vtkObject.h"

#if defined(__BORLANDC__)
/* Disable Borland compiler warning messages that often occur in valid code. */
#  pragma warn -8004 /* assigned a value that is never used */
#  pragma warn -8008 /* condition is always false */
#  pragma warn -8026 /* funcs w/class-by-value args not expanded inline */
#  pragma warn -8027 /* functions w/ do/for/while not expanded inline */
#  pragma warn -8060 /* possibly incorrect assignment */
#  pragma warn -8066 /* unreachable code */
#  pragma warn -8070 /* function should return a value */
#  pragma warn -8072 /* suspicious pointer arithmetic */
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  int cCreateEdgeFace( int, char*[] );
  int cReadEdgeFace( int, char*[] );

#ifdef __cplusplus
}
#endif /* __cplusplus */

inline int CreateEdgeFace( int argc, char* argv[] ) { return cCreateEdgeFace( argc, argv ); }
inline int   ReadEdgeFace( int argc, char* argv[] ) { return   cReadEdgeFace( argc, argv ); }

#endif /* __exodusII_test_h */
