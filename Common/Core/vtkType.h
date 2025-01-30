// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkType_h
#define vtkType_h

#include "vtkABINamespace.h"
#include "vtkCompiler.h"    // for VTK_USE_EXTERN_TEMPLATE
#include "vtkDeprecation.h" // For VTK_DEPRECATED_IN_9_5_0
#include "vtkOptions.h"     // for VTK_USE_64BIT_IDS, VTK_USE_64BIT_TIMESTAMPS, VTK_USE_FUTURE_BOOL
#include "vtk_kwiml.h"

#define VTK_SIZEOF_CHAR KWIML_ABI_SIZEOF_CHAR
#define VTK_SIZEOF_SHORT KWIML_ABI_SIZEOF_SHORT
#define VTK_SIZEOF_INT KWIML_ABI_SIZEOF_INT
#define VTK_SIZEOF_LONG KWIML_ABI_SIZEOF_LONG
#define VTK_SIZEOF_LONG_LONG KWIML_ABI_SIZEOF_LONG_LONG
#define VTK_SIZEOF_FLOAT KWIML_ABI_SIZEOF_FLOAT
#define VTK_SIZEOF_DOUBLE KWIML_ABI_SIZEOF_DOUBLE
#define VTK_SIZEOF_VOID_P KWIML_ABI_SIZEOF_DATA_PTR

/* Whether type "char" is signed (it may be signed or unsigned).  */
#if defined(KWIML_ABI_CHAR_IS_SIGNED)
#define VTK_TYPE_CHAR_IS_SIGNED 1
#else
#define VTK_TYPE_CHAR_IS_SIGNED 0
#endif

/*--------------------------------------------------------------------------*/
/* Define a unique integer identifier for each native scalar type.  */

/* These types are returned by GetDataType to indicate pixel type.  */
#define VTK_VOID 0
#define VTK_BIT 1
#define VTK_CHAR 2
#define VTK_SIGNED_CHAR 15
#define VTK_UNSIGNED_CHAR 3
#define VTK_SHORT 4
#define VTK_UNSIGNED_SHORT 5
#define VTK_INT 6
#define VTK_UNSIGNED_INT 7
#define VTK_LONG 8
#define VTK_UNSIGNED_LONG 9
#define VTK_FLOAT 10
#define VTK_DOUBLE 11
#define VTK_ID_TYPE 12

/* These types are not currently supported by GetDataType, but are for
   completeness.  */
#define VTK_STRING 13
#define VTK_OPAQUE 14

#define VTK_LONG_LONG 16
#define VTK_UNSIGNED_LONG_LONG 17

/* These types are required by vtkVariant and vtkVariantArray */
#define VTK_VARIANT 20
#define VTK_OBJECT 21

// deleted value
// #define VTK_UNICODE_STRING 22 <==== do not use

// vtkTypes.h can be included in C code directly, which does not support
// deprecation of enum values
#if defined(__cplusplus)
#define VTK_DEPRECATED_IN_9_5_0_TYPE(reason) VTK_DEPRECATED_IN_9_5_0(reason)
#else
#define VTK_DEPRECATED_IN_9_5_0_TYPE(reason)
#endif
/*--------------------------------------------------------------------------*/
// Define a unique integer identifier for each vtkDataObject type.
// When adding a new data type here, make sure to update vtkDataObjectTypes as well.
// Do not use values between 0 and current max, but add new values after the max,
// as values in between may already have been used in the past and been removed since.
enum vtkTypesDataObject
{
  VTK_POLY_DATA = 0,
  VTK_STRUCTURED_POINTS = 1,
  VTK_STRUCTURED_GRID = 2,
  VTK_RECTILINEAR_GRID = 3,
  VTK_UNSTRUCTURED_GRID = 4,
  VTK_PIECEWISE_FUNCTION = 5,
  VTK_IMAGE_DATA = 6,
  VTK_DATA_OBJECT = 7,
  VTK_DATA_SET = 8,
  VTK_POINT_SET = 9,
  VTK_UNIFORM_GRID = 10,
  VTK_COMPOSITE_DATA_SET = 11,
  VTK_MULTIGROUP_DATA_SET VTK_DEPRECATED_IN_9_5_0_TYPE("This type has been removed, do not use.") =
    12,
  VTK_MULTIBLOCK_DATA_SET = 13,
  VTK_HIERARCHICAL_DATA_SET VTK_DEPRECATED_IN_9_5_0_TYPE(
    "This type has been removed, do not use.") = 14,
  VTK_HIERARCHICAL_BOX_DATA_SET VTK_DEPRECATED_IN_9_5_0_TYPE(
    "This type has been removed, please use vtkOverlappingAMR instead.") = 15,
  VTK_GENERIC_DATA_SET = 16,
  VTK_HYPER_OCTREE VTK_DEPRECATED_IN_9_5_0_TYPE("This type has been removed, do not use.") = 17,
  VTK_TEMPORAL_DATA_SET VTK_DEPRECATED_IN_9_5_0_TYPE("This type has been removed, do not use.") =
    18,
  VTK_TABLE = 19,
  VTK_GRAPH = 20,
  VTK_TREE = 21,
  VTK_SELECTION = 22,
  VTK_DIRECTED_GRAPH = 23,
  VTK_UNDIRECTED_GRAPH = 24,
  VTK_MULTIPIECE_DATA_SET = 25,
  VTK_DIRECTED_ACYCLIC_GRAPH = 26,
  VTK_ARRAY_DATA = 27,
  VTK_REEB_GRAPH = 28,
  VTK_UNIFORM_GRID_AMR = 29,
  VTK_NON_OVERLAPPING_AMR = 30,
  VTK_OVERLAPPING_AMR = 31,
  VTK_HYPER_TREE_GRID = 32,
  VTK_MOLECULE = 33,
  VTK_PISTON_DATA_OBJECT VTK_DEPRECATED_IN_9_5_0_TYPE("This type has been removed, do not use.") =
    34,
  VTK_PATH = 35,
  VTK_UNSTRUCTURED_GRID_BASE = 36,
  VTK_PARTITIONED_DATA_SET = 37,
  VTK_PARTITIONED_DATA_SET_COLLECTION = 38,
  VTK_UNIFORM_HYPER_TREE_GRID = 39,
  VTK_EXPLICIT_STRUCTURED_GRID = 40,
  VTK_DATA_OBJECT_TREE = 41,
  VTK_ABSTRACT_ELECTRONIC_DATA = 42,
  VTK_OPEN_QUBE_ELECTRONIC_DATA = 43,
  VTK_ANNOTATION = 44,
  VTK_ANNOTATION_LAYERS = 45,
  VTK_BSP_CUTS = 46,
  VTK_GEO_JSON_FEATURE = 47,
  VTK_IMAGE_STENCIL_DATA = 48,
  VTK_CELL_GRID = 49
};

/*--------------------------------------------------------------------------*/
/* Define a casting macro for use by the constants below.  */
#if defined(__cplusplus)
#define VTK_TYPE_CAST(T, V) static_cast<T>(V)
#else
#define VTK_TYPE_CAST(T, V) ((T)(V))
#endif

/*--------------------------------------------------------------------------*/
/* Define min/max constants for each type.  */
#define VTK_BIT_MIN 0
#define VTK_BIT_MAX 1
#if VTK_TYPE_CHAR_IS_SIGNED
#define VTK_CHAR_MIN VTK_TYPE_CAST(char, 0x80)
#define VTK_CHAR_MAX VTK_TYPE_CAST(char, 0x7f)
#else
#define VTK_CHAR_MIN VTK_TYPE_CAST(char, 0u)
#define VTK_CHAR_MAX VTK_TYPE_CAST(char, 0xffu)
#endif
#define VTK_SIGNED_CHAR_MIN VTK_TYPE_CAST(signed char, 0x80)
#define VTK_SIGNED_CHAR_MAX VTK_TYPE_CAST(signed char, 0x7f)
#define VTK_UNSIGNED_CHAR_MIN VTK_TYPE_CAST(unsigned char, 0u)
#define VTK_UNSIGNED_CHAR_MAX VTK_TYPE_CAST(unsigned char, 0xffu)
#define VTK_SHORT_MIN VTK_TYPE_CAST(short, 0x8000)
#define VTK_SHORT_MAX VTK_TYPE_CAST(short, 0x7fff)
#define VTK_UNSIGNED_SHORT_MIN VTK_TYPE_CAST(unsigned short, 0u)
#define VTK_UNSIGNED_SHORT_MAX VTK_TYPE_CAST(unsigned short, 0xffffu)
#define VTK_INT_MIN VTK_TYPE_CAST(int, ~(~0u >> 1))
#define VTK_INT_MAX VTK_TYPE_CAST(int, ~0u >> 1)
#define VTK_UNSIGNED_INT_MIN VTK_TYPE_CAST(unsigned int, 0)
#define VTK_UNSIGNED_INT_MAX VTK_TYPE_CAST(unsigned int, ~0u)
#define VTK_LONG_MIN VTK_TYPE_CAST(long, ~(~0ul >> 1))
#define VTK_LONG_MAX VTK_TYPE_CAST(long, ~0ul >> 1)
#define VTK_UNSIGNED_LONG_MIN VTK_TYPE_CAST(unsigned long, 0ul)
#define VTK_UNSIGNED_LONG_MAX VTK_TYPE_CAST(unsigned long, ~0ul)
#define VTK_FLOAT_MIN VTK_TYPE_CAST(float, -1.0e+38f)
#define VTK_FLOAT_MAX VTK_TYPE_CAST(float, 1.0e+38f)
#define VTK_DOUBLE_MIN VTK_TYPE_CAST(double, -1.0e+299)
#define VTK_DOUBLE_MAX VTK_TYPE_CAST(double, 1.0e+299)
#define VTK_LONG_LONG_MIN VTK_TYPE_CAST(long long, ~(~0ull >> 1))
#define VTK_LONG_LONG_MAX VTK_TYPE_CAST(long long, ~0ull >> 1)
#define VTK_UNSIGNED_LONG_LONG_MIN VTK_TYPE_CAST(unsigned long long, 0ull)
#define VTK_UNSIGNED_LONG_LONG_MAX VTK_TYPE_CAST(unsigned long long, ~0ull)

/*--------------------------------------------------------------------------*/
/* Define named types and constants corresponding to specific integer
   and floating-point sizes and signedness.  */

/* Select an 8-bit integer type.  */
#if VTK_SIZEOF_CHAR == 1
typedef unsigned char vtkTypeUInt8;
typedef signed char vtkTypeInt8;
#define VTK_TYPE_UINT8 VTK_UNSIGNED_CHAR
#define VTK_TYPE_UINT8_MIN VTK_UNSIGNED_CHAR_MIN
#define VTK_TYPE_UINT8_MAX VTK_UNSIGNED_CHAR_MAX
#define VTK_TYPE_INT8 VTK_SIGNED_CHAR
#define VTK_TYPE_INT8_MIN VTK_SIGNED_CHAR_MIN
#define VTK_TYPE_INT8_MAX VTK_SIGNED_CHAR_MAX
#else
#error "No native data type can represent an 8-bit integer."
#endif

/* Select a 16-bit integer type.  */
#if VTK_SIZEOF_SHORT == 2
typedef unsigned short vtkTypeUInt16;
typedef signed short vtkTypeInt16;
#define VTK_TYPE_UINT16 VTK_UNSIGNED_SHORT
#define VTK_TYPE_UINT16_MIN VTK_UNSIGNED_SHORT_MIN
#define VTK_TYPE_UINT16_MAX VTK_UNSIGNED_SHORT_MAX
#define VTK_TYPE_INT16 VTK_SHORT
#define VTK_TYPE_INT16_MIN VTK_SHORT_MIN
#define VTK_TYPE_INT16_MAX VTK_SHORT_MAX
#elif VTK_SIZEOF_INT == 2
typedef unsigned int vtkTypeUInt16;
typedef signed int vtkTypeInt16;
#define VTK_TYPE_UINT16 VTK_UNSIGNED_INT
#define VTK_TYPE_UINT16_MIN VTK_UNSIGNED_INT_MIN
#define VTK_TYPE_UINT16_MAX VTK_UNSIGNED_INT_MAX
#define VTK_TYPE_INT16 VTK_INT
#define VTK_TYPE_INT16_MIN VTK_INT_MIN
#define VTK_TYPE_INT16_MAX VTK_INT_MAX
#else
#error "No native data type can represent a 16-bit integer."
#endif

/* Select a 32-bit integer type.  */
#if VTK_SIZEOF_INT == 4
typedef unsigned int vtkTypeUInt32;
typedef signed int vtkTypeInt32;
#define VTK_TYPE_UINT32 VTK_UNSIGNED_INT
#define VTK_TYPE_UINT32_MIN VTK_UNSIGNED_INT_MIN
#define VTK_TYPE_UINT32_MAX VTK_UNSIGNED_INT_MAX
#define VTK_TYPE_INT32 VTK_INT
#define VTK_TYPE_INT32_MIN VTK_INT_MIN
#define VTK_TYPE_INT32_MAX VTK_INT_MAX
#elif VTK_SIZEOF_LONG == 4
typedef unsigned long vtkTypeUInt32;
typedef signed long vtkTypeInt32;
#define VTK_TYPE_UINT32 VTK_UNSIGNED_LONG
#define VTK_TYPE_UINT32_MIN VTK_UNSIGNED_LONG_MIN
#define VTK_TYPE_UINT32_MAX VTK_UNSIGNED_LONG_MAX
#define VTK_TYPE_INT32 VTK_LONG
#define VTK_TYPE_INT32_MIN VTK_LONG_MIN
#define VTK_TYPE_INT32_MAX VTK_LONG_MAX
#else
#error "No native data type can represent a 32-bit integer."
#endif

/* Select a 64-bit integer type.  */
#if VTK_SIZEOF_LONG_LONG == 8
typedef unsigned long long vtkTypeUInt64;
typedef signed long long vtkTypeInt64;
#define VTK_TYPE_UINT64 VTK_UNSIGNED_LONG_LONG
#define VTK_TYPE_UINT64_MIN VTK_UNSIGNED_LONG_LONG_MIN
#define VTK_TYPE_UINT64_MAX VTK_UNSIGNED_LONG_LONG_MAX
#define VTK_TYPE_INT64 VTK_LONG_LONG
#define VTK_TYPE_INT64_MIN VTK_LONG_LONG_MIN
#define VTK_TYPE_INT64_MAX VTK_LONG_LONG_MAX
#elif VTK_SIZEOF_LONG == 8
typedef unsigned long vtkTypeUInt64;
typedef signed long vtkTypeInt64;
#define VTK_TYPE_UINT64 VTK_UNSIGNED_LONG
#define VTK_TYPE_UINT64_MIN VTK_UNSIGNED_LONG_MIN
#define VTK_TYPE_UINT64_MAX VTK_UNSIGNED_LONG_MAX
#define VTK_TYPE_INT64 VTK_LONG
#define VTK_TYPE_INT64_MIN VTK_LONG_MIN
#define VTK_TYPE_INT64_MAX VTK_LONG_MAX
#else
#error "No native data type can represent a 64-bit integer."
#endif

// If this is a 64-bit platform, or the user has indicated that 64-bit
// timestamps should be used, select an unsigned 64-bit integer type
// for use in MTime values. If possible, use 'unsigned long' as we have
// historically.
#if defined(VTK_USE_64BIT_TIMESTAMPS) || VTK_SIZEOF_VOID_P == 8
#if VTK_SIZEOF_LONG == 8
typedef unsigned long vtkMTimeType;
#define VTK_MTIME_TYPE_IMPL VTK_UNSIGNED_LONG
#define VTK_MTIME_MIN VTK_UNSIGNED_LONG_MIN
#define VTK_MTIME_MAX VTK_UNSIGNED_LONG_MAX
#else
typedef vtkTypeUInt64 vtkMTimeType;
#define VTK_MTIME_TYPE_IMPL VTK_TYPE_UINT64
#define VTK_MTIME_MIN VTK_TYPE_UINT64_MIN
#define VTK_MTIME_MAX VTK_TYPE_UINT64_MAX
#endif
#else
#if VTK_SIZEOF_LONG == 4
typedef unsigned long vtkMTimeType;
#define VTK_MTIME_TYPE_IMPL VTK_UNSIGNED_LONG
#define VTK_MTIME_MIN VTK_UNSIGNED_LONG_MIN
#define VTK_MTIME_MAX VTK_UNSIGNED_LONG_MAX
#else
typedef vtkTypeUInt32 vtkMTimeType;
#define VTK_MTIME_TYPE_IMPL VTK_TYPE_UINT32
#define VTK_MTIME_MIN VTK_TYPE_UINT32_MIN
#define VTK_MTIME_MAX VTK_TYPE_UINT32_MAX
#endif
#endif

/* Select a 32-bit floating point type.  */
#if VTK_SIZEOF_FLOAT == 4
typedef float vtkTypeFloat32;
#define VTK_TYPE_FLOAT32 VTK_FLOAT
#else
#error "No native data type can represent a 32-bit floating point value."
#endif

/* Select a 64-bit floating point type.  */
#if VTK_SIZEOF_DOUBLE == 8
typedef double vtkTypeFloat64;
#define VTK_TYPE_FLOAT64 VTK_DOUBLE
#else
#error "No native data type can represent a 64-bit floating point value."
#endif

/*--------------------------------------------------------------------------*/
/* Choose an implementation for vtkIdType.  */
#define VTK_HAS_ID_TYPE
#ifdef VTK_USE_64BIT_IDS
#if VTK_SIZEOF_LONG_LONG == 8
typedef long long vtkIdType;
#define VTK_ID_TYPE_IMPL VTK_LONG_LONG
#define VTK_SIZEOF_ID_TYPE VTK_SIZEOF_LONG_LONG
#define VTK_ID_MIN VTK_LONG_LONG_MIN
#define VTK_ID_MAX VTK_LONG_LONG_MAX
#define VTK_ID_TYPE_PRId "lld"
#elif VTK_SIZEOF_LONG == 8
typedef long vtkIdType;
#define VTK_ID_TYPE_IMPL VTK_LONG
#define VTK_SIZEOF_ID_TYPE VTK_SIZEOF_LONG
#define VTK_ID_MIN VTK_LONG_MIN
#define VTK_ID_MAX VTK_LONG_MAX
#define VTK_ID_TYPE_PRId "ld"
#else
#error "VTK_USE_64BIT_IDS is ON but no 64-bit integer type is available."
#endif
#else
typedef int vtkIdType;
#define VTK_ID_TYPE_IMPL VTK_INT
#define VTK_SIZEOF_ID_TYPE VTK_SIZEOF_INT
#define VTK_ID_MIN VTK_INT_MIN
#define VTK_ID_MAX VTK_INT_MAX
#define VTK_ID_TYPE_PRId "d"
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
#define VTK_TYPE_BOOL_TYPEDEFED
#if VTK_USE_FUTURE_BOOL
typedef bool vtkTypeBool;
typedef bool vtkTypeUBool;
#else
typedef int vtkTypeBool;
typedef unsigned int vtkTypeUBool;
#endif
#endif

#if defined(__cplusplus)
/* Description:
 * Returns true if data type tags a and b point to the same data type. This
 * is intended to handle vtkIdType, which does not have the same tag as its
 * underlying data type.
 * @note This method is only available when included from a C++ source file. */
VTK_ABI_NAMESPACE_BEGIN
inline vtkTypeBool vtkDataTypesCompare(int a, int b)
{
  return (a == b ||
    ((a == VTK_ID_TYPE || a == VTK_ID_TYPE_IMPL) && (b == VTK_ID_TYPE || b == VTK_ID_TYPE_IMPL)));
}
VTK_ABI_NAMESPACE_END
#endif

/*--------------------------------------------------------------------------*/
/** A macro to instantiate a template over all numerical types */
#define vtkInstantiateTemplateMacro(decl)                                                          \
  decl<float>;                                                                                     \
  decl<double>;                                                                                    \
  decl<char>;                                                                                      \
  decl<signed char>;                                                                               \
  decl<unsigned char>;                                                                             \
  decl<short>;                                                                                     \
  decl<unsigned short>;                                                                            \
  decl<int>;                                                                                       \
  decl<unsigned int>;                                                                              \
  decl<long>;                                                                                      \
  decl<unsigned long>;                                                                             \
  decl<long long>;                                                                                 \
  decl<unsigned long long>

#define vtkInstantiateSecondOrderTemplateMacro(decl0, decl1)                                       \
  decl0<decl1<float>>;                                                                             \
  decl0<decl1<double>>;                                                                            \
  decl0<decl1<char>>;                                                                              \
  decl0<decl1<signed char>>;                                                                       \
  decl0<decl1<unsigned char>>;                                                                     \
  decl0<decl1<short>>;                                                                             \
  decl0<decl1<unsigned short>>;                                                                    \
  decl0<decl1<int>>;                                                                               \
  decl0<decl1<unsigned int>>;                                                                      \
  decl0<decl1<long>>;                                                                              \
  decl0<decl1<unsigned long>>;                                                                     \
  decl0<decl1<long long>>;                                                                         \
  decl0<decl1<unsigned long long>>

#define vtkInstantiateStdFunctionTemplateMacro(decl0, decl1, delc2)                                \
  decl0<decl1<float(delc2)>>;                                                                      \
  decl0<decl1<double(delc2)>>;                                                                     \
  decl0<decl1<char(delc2)>>;                                                                       \
  decl0<decl1<signed char(delc2)>>;                                                                \
  decl0<decl1<unsigned char(delc2)>>;                                                              \
  decl0<decl1<short(delc2)>>;                                                                      \
  decl0<decl1<unsigned short(delc2)>>;                                                             \
  decl0<decl1<int(delc2)>>;                                                                        \
  decl0<decl1<unsigned int(delc2)>>;                                                               \
  decl0<decl1<long(delc2)>>;                                                                       \
  decl0<decl1<unsigned long(delc2)>>;                                                              \
  decl0<decl1<long long(delc2)>>;                                                                  \
  decl0<decl1<unsigned long long(delc2)>>

/** A macro to declare extern templates for all numerical types */
#ifdef VTK_USE_EXTERN_TEMPLATE
#define vtkExternTemplateMacro(decl) vtkInstantiateTemplateMacro(decl)
#define vtkExternSecondOrderTemplateMacro(decl0, decl1)                                            \
  vtkInstantiateSecondOrderTemplateMacro(decl0, decl1)
#define vtkExternStdFunctionTemplateMacro(decl0, decl1, decl2)                                     \
  vtkInstantiateStdFunctionTemplateMacro(decl0, decl1, decl2)
#else
#define vtkExternTemplateMacro(decl)
#define vtkExternSecondOrderTemplateMacro(decl0, decl1)
#define vtkExternStdFunctionTemplateMacro(decl0, decl1, decl2)
#endif

#endif
// VTK-HeaderTest-Exclude: vtkType.h
