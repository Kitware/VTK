// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTypeTraits
 * @brief   Template defining traits of native types used by VTK.
 *
 * vtkTypeTraits provides information about VTK's supported scalar types
 * that is useful for templates.
 */

#ifndef vtkTypeTraits_h
#define vtkTypeTraits_h

#include "vtkSystemIncludes.h"

// Forward-declare template.  There is no primary template.
VTK_ABI_NAMESPACE_BEGIN
template <class T>
struct vtkTypeTraits;

// Define a macro to simplify trait definitions.
#define VTK_TYPE_TRAITS(type, macro, isSigned, name, print, format)                                \
  template <>                                                                                      \
  struct vtkTypeTraits<type>                                                                       \
  {                                                                                                \
    /* The type itself.  */                                                                        \
    typedef type ValueType;                                                                        \
                                                                                                   \
    /* the value defined for this type in vtkType */                                               \
    enum                                                                                           \
    {                                                                                              \
      VTK_TYPE_ID = VTK_##macro                                                                    \
    };                                                                                             \
    static int VTKTypeID()                                                                         \
    {                                                                                              \
      return VTK_##macro;                                                                          \
    }                                                                                              \
                                                                                                   \
    /* The smallest possible value represented by the type.  */                                    \
    static type Min()                                                                              \
    {                                                                                              \
      return VTK_##macro##_MIN;                                                                    \
    }                                                                                              \
                                                                                                   \
    /* The largest possible value represented by the type.  */                                     \
    static type Max()                                                                              \
    {                                                                                              \
      return VTK_##macro##_MAX;                                                                    \
    }                                                                                              \
                                                                                                   \
    /* Whether the type is signed.  */                                                             \
    static int IsSigned()                                                                          \
    {                                                                                              \
      return isSigned;                                                                             \
    }                                                                                              \
                                                                                                   \
    /* An "alias" type that is the same size and signedness.  */                                   \
    typedef vtkType##name SizedType;                                                               \
                                                                                                   \
    /* A name for the type indicating its size and signedness.  */                                 \
    static const char* SizedName()                                                                 \
    {                                                                                              \
      return #name;                                                                                \
    }                                                                                              \
                                                                                                   \
    /* The common C++ name for the type (e.g. float, unsigned int, etc).*/                         \
    static const char* Name()                                                                      \
    {                                                                                              \
      return #type;                                                                                \
    }                                                                                              \
                                                                                                   \
    /* A type to use for printing or parsing values in strings.  */                                \
    typedef print PrintType;                                                                       \
                                                                                                   \
    /* A format for parsing values from strings.  Use with PrintType.  */                          \
    static const char* ParseFormat()                                                               \
    {                                                                                              \
      return format;                                                                               \
    }                                                                                              \
  }

// Define traits for floating-point types.
#define VTK_TYPE_NAME_FLOAT float
#define VTK_TYPE_NAME_DOUBLE double
#define VTK_TYPE_SIZED_FLOAT FLOAT32
#define VTK_TYPE_SIZED_DOUBLE FLOAT64
VTK_TYPE_TRAITS(float, FLOAT, 1, Float32, float, "%f");
VTK_TYPE_TRAITS(double, DOUBLE, 1, Float64, double, "%lf");

// Define traits for char types.
// Note the print type is short because not all platforms support formatting integers with char.
#define VTK_TYPE_NAME_CHAR char
#if VTK_TYPE_CHAR_IS_SIGNED
#define VTK_TYPE_SIZED_CHAR INT8
VTK_TYPE_TRAITS(char, CHAR, 1, Int8, short, "%hd");
#else
#define VTK_TYPE_SIZED_CHAR UINT8
VTK_TYPE_TRAITS(char, CHAR, 0, UInt8, unsigned short, "%hu");
#endif
#define VTK_TYPE_NAME_SIGNED_CHAR signed char
#define VTK_TYPE_NAME_UNSIGNED_CHAR unsigned char
#define VTK_TYPE_SIZED_SIGNED_CHAR INT8
#define VTK_TYPE_SIZED_UNSIGNED_CHAR UINT8
VTK_TYPE_TRAITS(signed char, SIGNED_CHAR, 1, Int8, short, "%hd");
VTK_TYPE_TRAITS(unsigned char, UNSIGNED_CHAR, 0, UInt8, unsigned short, "%hu");

// Define traits for short types.
#define VTK_TYPE_NAME_SHORT short
#define VTK_TYPE_NAME_UNSIGNED_SHORT unsigned short
#define VTK_TYPE_SIZED_SHORT INT16
#define VTK_TYPE_SIZED_UNSIGNED_SHORT UINT16
VTK_TYPE_TRAITS(short, SHORT, 1, Int16, short, "%hd");
VTK_TYPE_TRAITS(unsigned short, UNSIGNED_SHORT, 0, UInt16, unsigned short, "%hu");

// Define traits for int types.
#define VTK_TYPE_NAME_INT int
#define VTK_TYPE_NAME_UNSIGNED_INT unsigned int
#define VTK_TYPE_SIZED_INT INT32
#define VTK_TYPE_SIZED_UNSIGNED_INT UINT32
VTK_TYPE_TRAITS(int, INT, 1, Int32, int, "%d");
VTK_TYPE_TRAITS(unsigned int, UNSIGNED_INT, 0, UInt32, unsigned int, "%u");

// Define traits for long types.
#define VTK_TYPE_NAME_LONG long
#define VTK_TYPE_NAME_UNSIGNED_LONG unsigned long
#if VTK_SIZEOF_LONG == 4
#define VTK_TYPE_SIZED_LONG INT32
#define VTK_TYPE_SIZED_UNSIGNED_LONG UINT32
VTK_TYPE_TRAITS(long, LONG, 1, Int32, long, "%ld");
VTK_TYPE_TRAITS(unsigned long, UNSIGNED_LONG, 0, UInt32, unsigned long, "%lu");
#elif VTK_SIZEOF_LONG == 8
#define VTK_TYPE_SIZED_LONG INT64
#define VTK_TYPE_SIZED_UNSIGNED_LONG UINT64
VTK_TYPE_TRAITS(long, LONG, 1, Int64, long, "%ld");
VTK_TYPE_TRAITS(unsigned long, UNSIGNED_LONG, 0, UInt64, unsigned long, "%lu");
#else
#error "Type long is not 4 or 8 bytes in size."
#endif

// Define traits for long long types if they are enabled.
#define VTK_TYPE_NAME_LONG_LONG long long
#define VTK_TYPE_NAME_UNSIGNED_LONG_LONG unsigned long long
#if VTK_SIZEOF_LONG_LONG == 8
#define VTK_TYPE_SIZED_LONG_LONG INT64
#define VTK_TYPE_SIZED_UNSIGNED_LONG_LONG UINT64
#define VTK_TYPE_LONG_LONG_FORMAT "%ll"
VTK_TYPE_TRAITS(long long, LONG_LONG, 1, Int64, long long, VTK_TYPE_LONG_LONG_FORMAT "d");
VTK_TYPE_TRAITS(unsigned long long, UNSIGNED_LONG_LONG, 0, UInt64, unsigned long long,
  VTK_TYPE_LONG_LONG_FORMAT "u");
#undef VTK_TYPE_LONG_LONG_FORMAT
#else
#error "Type long long is not 8 bytes in size."
#endif

// Define traits for vtkIdType.  The template specialization is
// already defined for the corresponding native type.
#define VTK_TYPE_NAME_ID_TYPE vtkIdType
#if defined(VTK_USE_64BIT_IDS)
#define VTK_TYPE_SIZED_ID_TYPE INT64
#else
#define VTK_TYPE_SIZED_ID_TYPE INT32
#endif

#undef VTK_TYPE_TRAITS

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkTypeTraits.h
