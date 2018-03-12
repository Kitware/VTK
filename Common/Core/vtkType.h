/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkType.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkType_h
#define vtkType_h

#include "vtkConfigure.h"
#include "vtk_kwiml.h"

#define VTK_SIZEOF_CHAR KWIML_ABI_SIZEOF_CHAR
#define VTK_SIZEOF_SHORT KWIML_ABI_SIZEOF_SHORT
#define VTK_SIZEOF_INT KWIML_ABI_SIZEOF_INT
#define VTK_SIZEOF_LONG KWIML_ABI_SIZEOF_LONG
#define VTK_SIZEOF_LONG_LONG KWIML_ABI_SIZEOF_LONG_LONG
#define VTK_SIZEOF_FLOAT KWIML_ABI_SIZEOF_FLOAT
#define VTK_SIZEOF_DOUBLE KWIML_ABI_SIZEOF_DOUBLE
#define VTK_SIZEOF_VOID_P KWIML_ABI_SIZEOF_DATA_PTR

/* Whether type "long long" is enabled as a unique fundamental type.  */
#define VTK_TYPE_USE_LONG_LONG
#if VTK_SIZEOF_LONG_LONG == 0
# error "No 'long long' type available."
#endif

/* Whether type "char" is signed (it may be signed or unsigned).  */
#if defined(KWIML_ABI_CHAR_IS_SIGNED)
# define VTK_TYPE_CHAR_IS_SIGNED 1
#else
# define VTK_TYPE_CHAR_IS_SIGNED 0
#endif

/*--------------------------------------------------------------------------*/
/* Define a unique integer identifier for each native scalar type.  */

/* These types are returned by GetDataType to indicate pixel type.  */
#define VTK_VOID            0
#define VTK_BIT             1
#define VTK_CHAR            2
#define VTK_SIGNED_CHAR    15
#define VTK_UNSIGNED_CHAR   3
#define VTK_SHORT           4
#define VTK_UNSIGNED_SHORT  5
#define VTK_INT             6
#define VTK_UNSIGNED_INT    7
#define VTK_LONG            8
#define VTK_UNSIGNED_LONG   9
#define VTK_FLOAT          10
#define VTK_DOUBLE         11
#define VTK_ID_TYPE        12

/* These types are not currently supported by GetDataType, but are for
   completeness.  */
#define VTK_STRING         13
#define VTK_OPAQUE         14

#define VTK_LONG_LONG          16
#define VTK_UNSIGNED_LONG_LONG 17

#if !defined(VTK_LEGACY_REMOVE)

/* Legacy.  This type is never enabled.  */
#define VTK___INT64            18

/* Legacy.  This type is never enabled.  */
#define VTK_UNSIGNED___INT64   19

#endif

/* These types are required by vtkVariant and vtkVariantArray */
#define VTK_VARIANT 20
#define VTK_OBJECT 21

/* Storage for Unicode strings */
#define VTK_UNICODE_STRING 22

/*--------------------------------------------------------------------------*/
/* Define a unique integer identifier for each vtkDataObject type.          */
/* When adding a new data type here, make sure to update                    */
/* vtkDataObjectTypes as well.                                              */
#define VTK_POLY_DATA                       0
#define VTK_STRUCTURED_POINTS               1
#define VTK_STRUCTURED_GRID                 2
#define VTK_RECTILINEAR_GRID                3
#define VTK_UNSTRUCTURED_GRID               4
#define VTK_PIECEWISE_FUNCTION              5
#define VTK_IMAGE_DATA                      6
#define VTK_DATA_OBJECT                     7
#define VTK_DATA_SET                        8
#define VTK_POINT_SET                       9
#define VTK_UNIFORM_GRID                   10
#define VTK_COMPOSITE_DATA_SET             11
#define VTK_MULTIGROUP_DATA_SET            12
#define VTK_MULTIBLOCK_DATA_SET            13
#define VTK_HIERARCHICAL_DATA_SET          14
#define VTK_HIERARCHICAL_BOX_DATA_SET      15
#define VTK_GENERIC_DATA_SET               16
#define VTK_HYPER_OCTREE                   17
#define VTK_TEMPORAL_DATA_SET              18
#define VTK_TABLE                          19
#define VTK_GRAPH                          20
#define VTK_TREE                           21
#define VTK_SELECTION                      22
#define VTK_DIRECTED_GRAPH                 23
#define VTK_UNDIRECTED_GRAPH               24
#define VTK_MULTIPIECE_DATA_SET            25
#define VTK_DIRECTED_ACYCLIC_GRAPH         26
#define VTK_ARRAY_DATA                     27
#define VTK_REEB_GRAPH                     28
#define VTK_UNIFORM_GRID_AMR               29
#define VTK_NON_OVERLAPPING_AMR            30
#define VTK_OVERLAPPING_AMR                31
#define VTK_HYPER_TREE_GRID                32
#define VTK_MOLECULE                       33
#define VTK_PISTON_DATA_OBJECT             34
#define VTK_PATH                           35
#define VTK_UNSTRUCTURED_GRID_BASE         36
#define VTK_PARTITIONED_DATA_SET           37
#define VTK_PARTITIONED_DATA_SET_COLLECTION 38

/*--------------------------------------------------------------------------*/
/* Define a casting macro for use by the constants below.  */
#if defined(__cplusplus)
# define VTK_TYPE_CAST(T, V) static_cast< T >(V)
#else
# define VTK_TYPE_CAST(T, V) ((T)(V))
#endif

/*--------------------------------------------------------------------------*/
/* Define min/max constants for each type.  */
#define VTK_BIT_MIN                 0
#define VTK_BIT_MAX                 1
#if VTK_TYPE_CHAR_IS_SIGNED
# define VTK_CHAR_MIN               VTK_TYPE_CAST(char, 0x80)
# define VTK_CHAR_MAX               VTK_TYPE_CAST(char, 0x7f)
#else
# define VTK_CHAR_MIN               VTK_TYPE_CAST(char, 0u)
# define VTK_CHAR_MAX               VTK_TYPE_CAST(char, 0xffu)
#endif
#define VTK_SIGNED_CHAR_MIN         VTK_TYPE_CAST(signed char, 0x80)
#define VTK_SIGNED_CHAR_MAX         VTK_TYPE_CAST(signed char, 0x7f)
#define VTK_UNSIGNED_CHAR_MIN       VTK_TYPE_CAST(unsigned char, 0u)
#define VTK_UNSIGNED_CHAR_MAX       VTK_TYPE_CAST(unsigned char, 0xffu)
#define VTK_SHORT_MIN               VTK_TYPE_CAST(short, 0x8000)
#define VTK_SHORT_MAX               VTK_TYPE_CAST(short, 0x7fff)
#define VTK_UNSIGNED_SHORT_MIN      VTK_TYPE_CAST(unsigned short, 0u)
#define VTK_UNSIGNED_SHORT_MAX      VTK_TYPE_CAST(unsigned short, 0xffffu)
#define VTK_INT_MIN                 VTK_TYPE_CAST(int, ~(~0u >> 1))
#define VTK_INT_MAX                 VTK_TYPE_CAST(int, ~0u >> 1)
#define VTK_UNSIGNED_INT_MIN        VTK_TYPE_CAST(unsigned int, 0)
#define VTK_UNSIGNED_INT_MAX        VTK_TYPE_CAST(unsigned int, ~0u)
#define VTK_LONG_MIN                VTK_TYPE_CAST(long, ~(~0ul >> 1))
#define VTK_LONG_MAX                VTK_TYPE_CAST(long, ~0ul >> 1)
#define VTK_UNSIGNED_LONG_MIN       VTK_TYPE_CAST(unsigned long, 0ul)
#define VTK_UNSIGNED_LONG_MAX       VTK_TYPE_CAST(unsigned long, ~0ul)
#define VTK_FLOAT_MIN               VTK_TYPE_CAST(float, -1.0e+38f)
#define VTK_FLOAT_MAX               VTK_TYPE_CAST(float,  1.0e+38f)
#define VTK_DOUBLE_MIN              VTK_TYPE_CAST(double, -1.0e+299)
#define VTK_DOUBLE_MAX              VTK_TYPE_CAST(double,  1.0e+299)
#define VTK_LONG_LONG_MIN           VTK_TYPE_CAST(long long, ~(~0ull >> 1))
#define VTK_LONG_LONG_MAX           VTK_TYPE_CAST(long long, ~0ull >> 1)
#define VTK_UNSIGNED_LONG_LONG_MIN  VTK_TYPE_CAST(unsigned long long, 0ull)
#define VTK_UNSIGNED_LONG_LONG_MAX  VTK_TYPE_CAST(unsigned long long, ~0ull)

/*--------------------------------------------------------------------------*/
/* Define named types and constants corresponding to specific integer
   and floating-point sizes and signedness.  */

/* Select an 8-bit integer type.  */
#if VTK_SIZEOF_CHAR == 1
typedef unsigned char vtkTypeUInt8;
typedef signed char   vtkTypeInt8;
# define VTK_TYPE_UINT8 VTK_UNSIGNED_CHAR
# define VTK_TYPE_UINT8_MIN VTK_UNSIGNED_CHAR_MIN
# define VTK_TYPE_UINT8_MAX VTK_UNSIGNED_CHAR_MAX
# if VTK_TYPE_CHAR_IS_SIGNED
#  define VTK_TYPE_INT8 VTK_CHAR
#  define VTK_TYPE_INT8_MIN VTK_CHAR_MIN
#  define VTK_TYPE_INT8_MAX VTK_CHAR_MAX
# else
#  define VTK_TYPE_INT8 VTK_SIGNED_CHAR
#  define VTK_TYPE_INT8_MIN VTK_SIGNED_CHAR_MIN
#  define VTK_TYPE_INT8_MAX VTK_SIGNED_CHAR_MAX
# endif
#else
# error "No native data type can represent an 8-bit integer."
#endif

/* Select a 16-bit integer type.  */
#if VTK_SIZEOF_SHORT == 2
typedef unsigned short vtkTypeUInt16;
typedef signed short   vtkTypeInt16;
# define VTK_TYPE_UINT16 VTK_UNSIGNED_SHORT
# define VTK_TYPE_UINT16_MIN VTK_UNSIGNED_SHORT_MIN
# define VTK_TYPE_UINT16_MAX VTK_UNSIGNED_SHORT_MAX
# define VTK_TYPE_INT16 VTK_SHORT
# define VTK_TYPE_INT16_MIN VTK_SHORT_MIN
# define VTK_TYPE_INT16_MAX VTK_SHORT_MAX
#elif VTK_SIZEOF_INT == 2
typedef unsigned int vtkTypeUInt16;
typedef signed int   vtkTypeInt16;
# define VTK_TYPE_UINT16 VTK_UNSIGNED_INT
# define VTK_TYPE_UINT16_MIN VTK_UNSIGNED_INT_MIN
# define VTK_TYPE_UINT16_MAX VTK_UNSIGNED_INT_MAX
# define VTK_TYPE_INT16 VTK_INT
# define VTK_TYPE_INT16_MIN VTK_INT_MIN
# define VTK_TYPE_INT16_MAX VTK_INT_MAX
#else
# error "No native data type can represent a 16-bit integer."
#endif

/* Select a 32-bit integer type.  */
#if VTK_SIZEOF_INT == 4
typedef unsigned int vtkTypeUInt32;
typedef signed int   vtkTypeInt32;
# define VTK_TYPE_UINT32 VTK_UNSIGNED_INT
# define VTK_TYPE_UINT32_MIN VTK_UNSIGNED_INT_MIN
# define VTK_TYPE_UINT32_MAX VTK_UNSIGNED_INT_MAX
# define VTK_TYPE_INT32 VTK_INT
# define VTK_TYPE_INT32_MIN VTK_INT_MIN
# define VTK_TYPE_INT32_MAX VTK_INT_MAX
#elif VTK_SIZEOF_LONG == 4
typedef unsigned long vtkTypeUInt32;
typedef signed long   vtkTypeInt32;
# define VTK_TYPE_UINT32 VTK_UNSIGNED_LONG
# define VTK_TYPE_UINT32_MIN VTK_UNSIGNED_LONG_MIN
# define VTK_TYPE_UINT32_MAX VTK_UNSIGNED_LONG_MAX
# define VTK_TYPE_INT32 VTK_LONG
# define VTK_TYPE_INT32_MIN VTK_LONG_MIN
# define VTK_TYPE_INT32_MAX VTK_LONG_MAX
#else
# error "No native data type can represent a 32-bit integer."
#endif

/* Select a 64-bit integer type.  */
#if VTK_SIZEOF_LONG_LONG == 8
typedef unsigned long long vtkTypeUInt64;
typedef signed long long   vtkTypeInt64;
# define VTK_TYPE_UINT64 VTK_UNSIGNED_LONG_LONG
# define VTK_TYPE_UINT64_MIN VTK_UNSIGNED_LONG_LONG_MIN
# define VTK_TYPE_UINT64_MAX VTK_UNSIGNED_LONG_LONG_MAX
# define VTK_TYPE_INT64 VTK_LONG_LONG
# define VTK_TYPE_INT64_MIN VTK_LONG_LONG_MIN
# define VTK_TYPE_INT64_MAX VTK_LONG_LONG_MAX
#elif VTK_SIZEOF_LONG == 8
typedef unsigned long vtkTypeUInt64;
typedef signed long   vtkTypeInt64;
# define VTK_TYPE_UINT64 VTK_UNSIGNED_LONG
# define VTK_TYPE_UINT64_MIN VTK_UNSIGNED_LONG_MIN
# define VTK_TYPE_UINT64_MAX VTK_UNSIGNED_LONG_MAX
# define VTK_TYPE_INT64 VTK_LONG
# define VTK_TYPE_INT64_MIN VTK_LONG_MIN
# define VTK_TYPE_INT64_MAX VTK_LONG_MAX
#else
# error "No native data type can represent a 64-bit integer."
#endif

// Provide this define to facilitate apps that need to support older
// versions that do not have vtkMTimeType
// #ifndef VTK_HAS_MTIME_TYPE
// #if VTK_SIZEOF_LONG == 8
// typedef unsigned long vtkMTimeType;
// #else
// typedef vtkTypeUInt64 vtkMTimeType;
// #endif
// #endif
#define VTK_HAS_MTIME_TYPE

// If this is a 64-bit platform, or the user has indicated that 64-bit
// timestamps should be used, select an unsigned 64-bit integer type
// for use in MTime values. If possible, use 'unsigned long' as we have
// historically.
#if defined(VTK_USE_64BIT_TIMESTAMPS) || VTK_SIZEOF_VOID_P == 8
# if VTK_SIZEOF_LONG == 8
typedef unsigned long vtkMTimeType;
#  define VTK_MTIME_TYPE_IMPL VTK_UNSIGNED_LONG
#  define VTK_MTIME_MIN VTK_UNSIGNED_LONG_MIN
#  define VTK_MTIME_MAX VTK_UNSIGNED_LONG_MAX
# else
typedef vtkTypeUInt64 vtkMTimeType;
#  define VTK_MTIME_TYPE_IMPL VTK_TYPE_UINT64
#  define VTK_MTIME_MIN VTK_TYPE_UINT64_MIN
#  define VTK_MTIME_MAX VTK_TYPE_UINT64_MAX
# endif
#else
# if VTK_SIZEOF_LONG == 4
typedef unsigned long vtkMTimeType;
#  define VTK_MTIME_TYPE_IMPL VTK_UNSIGNED_LONG
#  define VTK_MTIME_MIN VTK_UNSIGNED_LONG_MIN
#  define VTK_MTIME_MAX VTK_UNSIGNED_LONG_MAX
# else
typedef vtkTypeUInt32 vtkMTimeType;
#  define VTK_MTIME_TYPE_IMPL VTK_TYPE_UINT32
#  define VTK_MTIME_MIN VTK_TYPE_UINT32_MIN
#  define VTK_MTIME_MAX VTK_TYPE_UINT32_MAX
# endif
#endif

/* Select a 32-bit floating point type.  */
#if VTK_SIZEOF_FLOAT == 4
typedef float vtkTypeFloat32;
# define VTK_TYPE_FLOAT32 VTK_FLOAT
#else
# error "No native data type can represent a 32-bit floating point value."
#endif

/* Select a 64-bit floating point type.  */
#if VTK_SIZEOF_DOUBLE == 8
typedef double vtkTypeFloat64;
# define VTK_TYPE_FLOAT64 VTK_DOUBLE
#else
# error "No native data type can represent a 64-bit floating point value."
#endif

/*--------------------------------------------------------------------------*/
/* Choose an implementation for vtkIdType.  */
#define VTK_HAS_ID_TYPE
#ifdef VTK_USE_64BIT_IDS
# if VTK_SIZEOF_LONG_LONG == 8
typedef long long vtkIdType;
#  define VTK_ID_TYPE_IMPL VTK_LONG_LONG
#  define VTK_SIZEOF_ID_TYPE VTK_SIZEOF_LONG_LONG
#  define VTK_ID_MIN VTK_LONG_LONG_MIN
#  define VTK_ID_MAX VTK_LONG_LONG_MAX
#  define VTK_ID_TYPE_PRId "lld"
# elif VTK_SIZEOF_LONG == 8
typedef long vtkIdType;
#  define VTK_ID_TYPE_IMPL VTK_LONG
#  define VTK_SIZEOF_ID_TYPE VTK_SIZEOF_LONG
#  define VTK_ID_MIN VTK_LONG_MIN
#  define VTK_ID_MAX VTK_LONG_MAX
#  define VTK_ID_TYPE_PRId "ld"
# else
#  error "VTK_USE_64BIT_IDS is ON but no 64-bit integer type is available."
# endif
#else
typedef int vtkIdType;
# define VTK_ID_TYPE_IMPL VTK_INT
# define VTK_SIZEOF_ID_TYPE VTK_SIZEOF_INT
# define VTK_ID_MIN VTK_INT_MIN
# define VTK_ID_MAX VTK_INT_MAX
# define VTK_ID_TYPE_PRId "d"
#endif

#ifndef __cplusplus
  // Make sure that when VTK headers are used by the C compiler we make
  // sure to define the bool type. This is possible when using IO features
  // like vtkXMLWriterC.h
  #include "stdbool.h"
#endif

/*--------------------------------------------------------------------------*/
/* If not already defined, define vtkTypeBool. When VTK was started, some   */
/* compilers did not yet support the bool type, and so VTK often used int,  */
/* or more rarely unsigned int, where it should have used bool.             */
/* Eventually vtkTypeBool will switch to real bool.                         */
#ifndef VTK_TYPE_BOOL_TYPEDEFED
# define VTK_TYPE_BOOL_TYPEDEFED
# if 1
   typedef int vtkTypeBool;
   typedef unsigned int vtkTypeUBool;
# else
   typedef bool vtkTypeBool;
   typedef bool vtkTypeUBool;
# endif
#endif


#if defined(__cplusplus)
/* Description:
 * Returns true if data type tags a and b point to the same data type. This
 * is intended to handle vtkIdType, which does not have the same tag as its
 * underlying data type.
 * @note This method is only available when included from a C++ source file. */
inline vtkTypeBool vtkDataTypesCompare(int a, int b)
{
  return (a == b ||
          ((a == VTK_ID_TYPE || a == VTK_ID_TYPE_IMPL) &&
           (b == VTK_ID_TYPE || b == VTK_ID_TYPE_IMPL)));
}
#endif

/*--------------------------------------------------------------------------*/
/** A macro to instantiate a template over all numerical types */
#define vtkInstantiateTemplateMacro(decl) \
  decl<float>; \
  decl<double>; \
  decl<char>; \
  decl<signed char>; \
  decl<unsigned char>; \
  decl<short>; \
  decl<unsigned short>; \
  decl<int>; \
  decl<unsigned int>; \
  decl<long>; \
  decl<unsigned long>; \
  decl<long long>; \
  decl<unsigned long long>;

/** A macro to declare extern templates for all numerical types */
#ifdef VTK_USE_EXTERN_TEMPLATE
#define vtkExternTemplateMacro(decl) \
  vtkInstantiateTemplateMacro(decl)
#else
#define vtkExternTemplateMacro(decl)
#endif

#endif
// VTK-HeaderTest-Exclude: vtkType.h
