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

/* These types are enabled if VTK_TYPE_USE_LONG_LONG is defined.  */
#define VTK_LONG_LONG          16
#define VTK_UNSIGNED_LONG_LONG 17

/* This type is enabled if VTK_TYPE_USE___INT64 is defined.  */
#define VTK___INT64            18

/* This type is enabled if VTK_TYPE_USE___INT64 and
   VTK_TYPE_CONVERT_UI64_TO_DOUBLE are both defined.  */
#define VTK_UNSIGNED___INT64   19

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
#if defined(VTK_SIZEOF_LONG_LONG)
# define VTK_LONG_LONG_MIN          VTK_TYPE_CAST(long long, ~(~0ull >> 1))
# define VTK_LONG_LONG_MAX          VTK_TYPE_CAST(long long, ~0ull >> 1)
# define VTK_UNSIGNED_LONG_LONG_MIN VTK_TYPE_CAST(unsigned long long, 0ull)
# define VTK_UNSIGNED_LONG_LONG_MAX VTK_TYPE_CAST(unsigned long long, ~0ull)
#endif
#if defined(VTK_SIZEOF___INT64)
# if defined(VTK_TYPE_SAME_LONG_AND___INT64)
#  define VTK___INT64_MIN           VTK_TYPE_CAST(__int64, ~(~0ul >> 1))
#  define VTK___INT64_MAX           VTK_TYPE_CAST(__int64, ~0ul >> 1)
#  define VTK_UNSIGNED___INT64_MIN  VTK_TYPE_CAST(unsigned __int64, 0ul)
#  define VTK_UNSIGNED___INT64_MAX  VTK_TYPE_CAST(unsigned __int64, ~0ul)
# elif defined(VTK_TYPE_SAME_LONG_LONG_AND___INT64)
#  define VTK___INT64_MIN           VTK_TYPE_CAST(__int64, ~(~0ull >> 1))
#  define VTK___INT64_MAX           VTK_TYPE_CAST(__int64, ~0ull >> 1)
#  define VTK_UNSIGNED___INT64_MIN  VTK_TYPE_CAST(unsigned __int64, 0ull)
#  define VTK_UNSIGNED___INT64_MAX  VTK_TYPE_CAST(unsigned __int64, ~0ull)
# else
#  define VTK___INT64_MIN           VTK_TYPE_CAST(__int64, ~(~0ui64 >> 1))
#  define VTK___INT64_MAX           VTK_TYPE_CAST(__int64, ~0ui64 >> 1)
#  define VTK_UNSIGNED___INT64_MIN  VTK_TYPE_CAST(unsigned __int64, 0ui64)
#  define VTK_UNSIGNED___INT64_MAX  VTK_TYPE_CAST(unsigned __int64, ~0ui64)
# endif
#endif

/* Define compatibility names for these constants.  */
#if !defined(VTK_LEGACY_REMOVE)
# define VTK_LARGE_INTEGER VTK_INT_MAX
# define VTK_LARGE_FLOAT VTK_FLOAT_MAX
#endif

/*--------------------------------------------------------------------------*/
/* Define named types and constants corresponding to specific integer
   and floating-point sizes and signedness.  */

/* Select an 8-bit integer type.  */
#if VTK_SIZEOF_CHAR == 1
typedef unsigned char vtkTypeUInt8;
typedef signed char   vtkTypeInt8;
# define VTK_TYPE_UINT8 VTK_UNSIGNED_CHAR
# if VTK_TYPE_CHAR_IS_SIGNED
#  define VTK_TYPE_INT8 VTK_CHAR
# else
#  define VTK_TYPE_INT8 VTK_SIGNED_CHAR
# endif
#else
# error "No native data type can represent an 8-bit integer."
#endif

/* Select a 16-bit integer type.  */
#if VTK_SIZEOF_SHORT == 2
typedef unsigned short vtkTypeUInt16;
typedef signed short   vtkTypeInt16;
# define VTK_TYPE_UINT16 VTK_UNSIGNED_SHORT
# define VTK_TYPE_INT16 VTK_SHORT
#elif VTK_SIZEOF_INT == 2
typedef unsigned int vtkTypeUInt16;
typedef signed int   vtkTypeInt16;
# define VTK_TYPE_UINT16 VTK_UNSIGNED_INT
# define VTK_TYPE_INT16 VTK_INT
#else
# error "No native data type can represent a 16-bit integer."
#endif

/* Select a 32-bit integer type.  */
#if VTK_SIZEOF_INT == 4
typedef unsigned int vtkTypeUInt32;
typedef signed int   vtkTypeInt32;
# define VTK_TYPE_UINT32 VTK_UNSIGNED_INT
# define VTK_TYPE_INT32 VTK_INT
#elif VTK_SIZEOF_LONG == 4
typedef unsigned long vtkTypeUInt32;
typedef signed long   vtkTypeInt32;
# define VTK_TYPE_UINT32 VTK_UNSIGNED_LONG
# define VTK_TYPE_INT32 VTK_LONG
#else
# error "No native data type can represent a 32-bit integer."
#endif

/* Select a 64-bit integer type.  */
#if defined(VTK_TYPE_USE_LONG_LONG) && VTK_SIZEOF_LONG_LONG == 8
typedef unsigned long long vtkTypeUInt64;
typedef signed long long   vtkTypeInt64;
# define VTK_TYPE_UINT64 VTK_UNSIGNED_LONG_LONG
# define VTK_TYPE_INT64 VTK_LONG_LONG
#elif VTK_SIZEOF_LONG == 8
typedef unsigned long vtkTypeUInt64;
typedef signed long   vtkTypeInt64;
# define VTK_TYPE_UINT64 VTK_UNSIGNED_LONG
# define VTK_TYPE_INT64 VTK_LONG
#elif defined(VTK_TYPE_USE___INT64) && VTK_SIZEOF___INT64 == 8
typedef unsigned __int64 vtkTypeUInt64;
typedef signed __int64   vtkTypeInt64;
# define VTK_TYPE_UINT64 VTK_UNSIGNED___INT64
# define VTK_TYPE_INT64 VTK___INT64
#else
# error "No native data type can represent a 64-bit integer."
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
# if defined(VTK_SIZEOF_LONG) && VTK_SIZEOF_LONG == 8 && 0
typedef long vtkIdType;
#  define VTK_SIZEOF_ID_TYPE VTK_SIZEOF_LONG
#  define VTK_ID_MIN VTK_LONG_MIN
#  define VTK_ID_MAX VTK_LONG_MAX
# elif defined(VTK_TYPE_USE_LONG_LONG) && VTK_SIZEOF_LONG_LONG == 8
typedef long long vtkIdType;
#  define VTK_SIZEOF_ID_TYPE VTK_SIZEOF_LONG_LONG
#  define VTK_ID_MIN VTK_LONG_LONG_MIN
#  define VTK_ID_MAX VTK_LONG_LONG_MAX
# elif defined(VTK_TYPE_USE___INT64) && VTK_SIZEOF___INT64 == 8
typedef __int64 vtkIdType;
#  define VTK_SIZEOF_ID_TYPE VTK_SIZEOF___INT64
#  define VTK_ID_MIN VTK___INT64_MIN
#  define VTK_ID_MAX VTK___INT64_MAX
# else
#  error "VTK_USE_64BIT_IDS is ON but no 64-bit integer type is available."
# endif
#else
typedef int vtkIdType;
# define VTK_SIZEOF_ID_TYPE VTK_SIZEOF_INT
# define VTK_ID_MIN VTK_INT_MIN
# define VTK_ID_MAX VTK_INT_MAX
#endif

/*--------------------------------------------------------------------------*/
/* Provide deprecated pre-VTK6 constant. */
#if !defined(VTK_LEGACY_REMOVE)
#  define VTK_LARGE_ID VTK_ID_MAX
#endif

/*--------------------------------------------------------------------------*/
/* Define the type of floating point interface used for old and new
   versions of VTK.  VTK 4.2 and older use float and VTK 4.4 and newer
   use double for most of the API calls.  */
#if !defined(VTK_LEGACY_REMOVE)
#  define vtkFloatingPointType double
#endif

#endif
// VTK-HeaderTest-Exclude: vtkType.h
