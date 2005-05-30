/*=========================================================================

  Program:   ParaView
  Module:    vtkTypeTraits.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTypeTraits - Template defining traits of native types used by VTK.
// .SECTION Description
// vtkTypeTraits provides information about VTK's supported scalar types
// that is useful for templates.
#ifndef __vtkTypeTraits_h
#define __vtkTypeTraits_h

#include "vtkSystemIncludes.h"

// Forward-declare template.  There is no primary template.
template <class T> struct vtkTypeTraits;

// Define a macro to simplify trait definitions.
#define VTK_TYPE_TRAITS(type, macro, isSigned, name, print, format)           \
  VTK_TEMPLATE_SPECIALIZE struct vtkTypeTraits< type >                        \
  {                                                                           \
    /* The type itself.  */                                                   \
    typedef type ValueType;                                                   \
                                                                              \
    /* The smallest possible value represented by the type.  */               \
    static type Min() { return VTK_##macro##_MIN; }                           \
                                                                              \
    /* The largest possible value represented by the type.  */                \
    static type Max() { return VTK_##macro##_MAX; }                           \
                                                                              \
    /* Whether the type is signed.  */                                        \
    static int IsSigned() { return isSigned; }                                \
                                                                              \
    /* An \"alias\" type that is the same size and signedness.  */            \
    typedef vtkType##name SizedType;                                          \
                                                                              \
    /* A name for the type indicating its size and signedness.  */            \
    static const char* SizedName() { return #name; }                          \
                                                                              \
    /* A type to use for printing or parsing values in strings.  */           \
    typedef print PrintType;                                                  \
                                                                              \
    /* A format for parsing values from strings.  Use with PrintType.  */     \
    static const char* ParseFormat() { return format; }                       \
  }

// Define traits for floating-point types.
VTK_TYPE_TRAITS(float, FLOAT, 1, Float32, float, "%f");
VTK_TYPE_TRAITS(double, DOUBLE, 1, Float64, double, "%lf");

// Define traits for char types.
#if VTK_TYPE_CHAR_IS_SIGNED
VTK_TYPE_TRAITS(char, CHAR, 1, Int8, short, "%hd");
#else
VTK_TYPE_TRAITS(char, CHAR, 0, UInt8, unsigned short, "%hu");
#endif
VTK_TYPE_TRAITS(signed char, SIGNED_CHAR, 1, Int8, short, "%hd");
VTK_TYPE_TRAITS(unsigned char, UNSIGNED_CHAR, 0, UInt8, unsigned short, "%hu");

// Define traits for short types.
VTK_TYPE_TRAITS(short, SHORT, 1, Int16, short, "%hd");
VTK_TYPE_TRAITS(unsigned short, UNSIGNED_SHORT, 0, UInt16, unsigned short,
                "%hu");

// Define traits for int types.
VTK_TYPE_TRAITS(int, INT, 1, Int32, int, "%d");
VTK_TYPE_TRAITS(unsigned int, UNSIGNED_INT, 0, UInt32, unsigned int, "%u");

// Define traits for long types.
#if VTK_SIZEOF_LONG == 4
VTK_TYPE_TRAITS(long, LONG, 1, Int32, long, "%ld");
VTK_TYPE_TRAITS(unsigned long, UNSIGNED_LONG, 0, UInt32, unsigned long, "%lu");
#elif VTK_SIZEOF_LONG == 8
VTK_TYPE_TRAITS(long, LONG, 1, Int64, long, "%ld");
VTK_TYPE_TRAITS(unsigned long, UNSIGNED_LONG, 0, UInt64, unsigned long, "%lu");
#else
# error "Type long is not 4 or 8 bytes in size."
#endif

// Define traits for long long types if they are enabled.
#if defined(VTK_TYPE_USE_LONG_LONG)
# if VTK_SIZEOF_LONG_LONG == 8
VTK_TYPE_TRAITS(long long, LONG_LONG, 1, Int64, long long, "%lld");
VTK_TYPE_TRAITS(unsigned long long, UNSIGNED_LONG_LONG, 0, UInt64,
                unsigned long long, "%llu");
# else
#  error "Type long long is not 8 bytes in size."
# endif
#endif

// Define traits for __int64 types if they are enabled.
#if defined(VTK_TYPE_USE___INT64)
# if VTK_SIZEOF___INT64 == 8
VTK_TYPE_TRAITS(__int64, __INT64, 1, Int64, __int64, "%I64d");
VTK_TYPE_TRAITS(unsigned __int64, UNSIGNED___INT64, 0, UInt64,
                unsigned __int64, "%I64u");
# else
#  error "Type __int64 is not 8 bytes in size."
# endif
#endif

#undef VTK_TYPE_TRAITS

#endif
