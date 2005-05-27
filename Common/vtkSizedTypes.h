/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSizedTypes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME SizedTypes - Definitions of types of specific sizes
// .SECTION Description
// This header file defines types with names that denote their
// size and sign, for example vtkInt32Type, vtkUnsignedInt8Type,
// vtkFloat64Type.

#ifndef __vtkSizedTypes_h
#define __vtkSizedTypes_h

#include "vtkSystemIncludes.h"

// individually define all the types that will be available
// (maybe eventually make these advanced options under CMake)
#define VTK_USE_INT8
#define VTK_USE_UINT8
#define VTK_USE_INT16
#define VTK_USE_UINT16
#define VTK_USE_INT32
#define VTK_USE_UINT32
#define VTK_USE_FLOAT32
#define VTK_USE_FLOAT64

#if defined(VTK_SIZEOF_LONG_LONG)
# define VTK_USE_INT64
# define VTK_USE_UINT64
#elif defined(VTK_SIZEOF___INT64)
# define VTK_USE_INT64
# if defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
#  define VTK_USE_UINT64
# endif
#endif

//--------------------------------------------------------------------------
// A macro to evaluate an expression for all scalar types.
// Because the types are sized, duplication of code is avoided,
// e.g. if "int" and "long" are both 32bit, then only one
// template instantiation is done instead of two.
// Also, the VTK_USE_INT32 options can be eventually be used to
// manually turn on or off instantiations for certain sized types
// at compile time.

#define vtkSizedTemplateMacro(call) \
  vtkTypeMacroCase_d(VTK_DOUBLE, vtkDoubleAliasType, call);              \
  vtkTypeMacroCase_f(VTK_FLOAT, vtkFloatAliasType, call);                \
  vtkTypeMacroCase_ll(VTK_LONG_LONG, vtkLongLongAliasType, call)         \
  vtkTypeMacroCase_ull(VTK_UNSIGNED_LONG_LONG, vtkUnsignedLongLongAliasType, call) \
  vtkTypeMacroCase_i64(VTK___INT64, vtk__int64AliasType, call)           \
  vtkTypeMacroCase_ui64(VTK_UNSIGNED___INT64, vtk__int64AliasType, call) \
  vtkTypeMacroCase_id(VTK_ID_TYPE, vtkIdTypeAliasType, call);            \
  vtkTypeMacroCase_l(VTK_LONG, vtkLongAliasType, call);                  \
  vtkTypeMacroCase_ul(VTK_UNSIGNED_LONG, vtkUnsignedLongAliasType, call);\
  vtkTypeMacroCase_i(VTK_INT, vtkIntAliasType, call);                    \
  vtkTypeMacroCase_ui(VTK_UNSIGNED_INT, vtkUnsignedIntAliasType, call);  \
  vtkTypeMacroCase_s(VTK_SHORT, vtkShortAliasType, call);                \
  vtkTypeMacroCase_us(VTK_UNSIGNED_SHORT, vtkUnsignedShortAliasType, call);\
  vtkTypeMacroCase_c(VTK_CHAR, vtkCharAliasType, call);                  \
  vtkTypeMacroCase_sc(VTK_SIGNED_CHAR, vtkSignedCharAliasType, call);    \
  vtkTypeMacroCase_uc(VTK_UNSIGNED_CHAR, vtkUnsignedCharAliasType, call)

#define vtkTypeMacroCase(typeN, type, call)     \
  case typeN: { typedef type VTK_TT; call; }; break

#define vtkMissingTypeWarningMacro(typeN, x) \
  case typeN: vtkGenericWarningMacro(x); break

//--------------------------------------------------------------------------
// make the typedefs for all the sized types

#ifdef VTK_USE_INT8
# if (VTK_SIZEOF_CHAR == 1)
typedef signed char vtkInt8Type;
#  define VTK_INT8_TYPE VTK_SIGNED_CHAR
#  define VTK_INT8_MIN VTK_SIGNED_CHAR_MIN
#  define VTK_INT8_MAX VTK_SIGNED_CHAR_MAX
# else
#  error "No native data type can be used for vtkInt8Type."
# endif
#endif

#ifdef VTK_USE_UINT8
# if (VTK_SIZEOF_CHAR == 1)
typedef unsigned char vtkUInt8Type;
#  define VTK_UINT8_TYPE VTK_UNSIGNED_CHAR
#  define VTK_UINT8_MIN VTK_UNSIGNED_CHAR_MIN
#  define VTK_UINT8_MAX VTK_UNSIGNED_CHAR_MAX
# else
#  error "No native data type can be used for vtkUInt8Type."
# endif
#endif

#ifdef VTK_USE_INT16
# if (VTK_SIZEOF_SHORT == 2)
typedef short vtkInt16Type;
#  define VTK_INT16_TYPE VTK_SHORT
#  define VTK_INT16_MIN VTK_SHORT_MIN
#  define VTK_INT16_MAX VTK_SHORT_MAX
# else
#  error "No native data type can be used for vtkInt16Type."
# endif
#endif

#ifdef VTK_USE_UINT16
# if (VTK_SIZEOF_SHORT == 2)
typedef unsigned short vtkUInt16Type;
#  define VTK_UINT16_TYPE VTK_UNSIGNED_SHORT
#  define VTK_UINT16_MIN VTK_UNSIGNED_SHORT_MIN
#  define VTK_UINT16_MAX VTK_UNSIGNED_SHORT_MAX
# else
#  error "No native data type can be used for vtkUInt16Type."
# endif
#endif

#ifdef VTK_USE_INT32
# if (VTK_SIZEOF_INT == 4)
typedef int vtkInt32Type;
#  define VTK_INT32_TYPE VTK_INT
#  define VTK_INT32_MIN VTK_INT_MIN
#  define VTK_INT32_MAX VTK_INT_MAX
# elif (VTK_SIZEOF_LONG == 4)
typedef long vtkInt32Type;
#  define VTK_INT32_TYPE VTK_LONG
#  define VTK_INT32_MIN VTK_LONG_MIN
#  define VTK_INT32_MAX VTK_LONG_MAX
# else
#  error "No native data type can be used for vtkInt32Type."
# endif
#endif

#ifdef VTK_USE_UINT32
# if (VTK_SIZEOF_INT == 4)
typedef unsigned int vtkUInt32Type;
#  define VTK_UINT32_TYPE VTK_UNSIGNED_INT
#  define VTK_UINT32_MIN VTK_UNSIGNED_INT_MIN
#  define VTK_UINT32_MAX VTK_UNSIGNED_INT_MAX
# elif (VTK_SIZEOF_LONG == 4)
typedef unsigned long vtkUInt32Type;
#  define VTK_UINT32_TYPE VTK_UNSIGNED_LONG
#  define VTK_UINT32_MIN VTK_UNSIGNED_LONG_MIN
#  define VTK_UINT32_MAX VTK_UNSIGNED_LONG_MAX
# else
#  error "No native data type can be used for vtkUInt32Type."
# endif
#endif

#ifdef VTK_USE_INT64
# if (VTK_SIZEOF_LONG == 8)
typedef long vtkInt64Type;
#  define VTK_INT64_TYPE VTK_LONG
#  define VTK_INT64_MIN VTK_LONG_MIN
#  define VTK_INT64_MAX VTK_LONG_MAX
# elif defined(VTK_SIZEOF_LONG_LONG)
#  if (VTK_SIZEOF_LONG_LONG == 8)
typedef long long vtkInt64Type;
#   define VTK_INT64_TYPE VTK_LONG_LONG
#   define VTK_INT64_MIN VTK_LONG_LONG_MIN
#   define VTK_INT64_MAX VTK_LONG_LONG_MAX
#  endif
# elif defined(VTK_SIZEOF___INT64)
typedef __int64 vtkInt64Type;
#   define VTK_INT64_TYPE VTK___INT64
#   define VTK_INT64_MIN VTK___INT64_MIN
#   define VTK_INT64_MAX VTK___INT64_MAX
# else
#  error "No native data type can be used for vtkInt64Type."
# endif
#endif

#ifdef VTK_USE_UINT64
# if (VTK_SIZEOF_LONG == 8)
typedef unsigned long vtkUInt64Type;
#  define VTK_UINT64_TYPE VTK_UNSIGNED_LONG
#  define VTK_UINT64_MIN VTK_UNSIGNED_LONG_MIN
#  define VTK_UINT64_MAX VTK_UNSIGNED_LONG_MAX
# elif defined(VTK_SIZEOF_LONG_LONG)
#  if (VTK_SIZEOF_LONG_LONG == 8)
typedef unsigned long long vtkUInt64Type;
#   define VTK_UINT64_TYPE VTK_UNSIGNED_LONG_LONG
#   define VTK_UINT64_MIN VTK_UNSIGNED_LONG_LONG_MIN
#   define VTK_UINT64_MAX VTK_UNSIGNED_LONG_LONG_MAX
#  endif
# elif defined(VTK_SIZEOF___INT64) && defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
typedef unsigned __int64 vtkUInt64Type;
#  define VTK_UINT64_TYPE VTK_UNSIGNED___INT64
#  define VTK_UINT64_MIN VTK_UNSIGNED___INT64_MIN
#  define VTK_UINT64_MAX VTK_UNSIGNED___INT64_MAX
# else
#  error "No native data type can be used for vtkUInt64Type."
# endif
#endif

#ifdef VTK_USE_FLOAT32
# if (VTK_SIZEOF_FLOAT == 4)
typedef float vtkFloat32Type;
#  define VTK_FLOAT32_TYPE VTK_FLOAT
#  define VTK_FLOAT32_MIN VTK_FLOAT_MIN
#  define VTK_FLOAT32_MAX VTK_FLOAT_MAX
# else
#  error "No native data type can be used for vtkFloat32Type."
# endif
#endif

#ifdef VTK_USE_FLOAT64
# if (VTK_SIZEOF_DOUBLE == 8)
typedef double vtkFloat64Type;
#  define VTK_FLOAT64_TYPE VTK_DOUBLE
#  define VTK_FLOAT64_MIN VTK_DOUBLE_MIN
#  define VTK_FLOAT64_MAX VTK_DOUBLE_MAX
# else
#  error "No native data type can be used for vtkFloat64Type."
# endif
#endif

//--------------------------------------------------------------------------
// alias the fundamental types to the new, more useful types

#if defined(VTK_USE_INT8) && (VTK_SIZEOF_CHAR == 1)
typedef vtkInt8Type vtkSignedCharAliasType;
# define vtkTypeMacroCase_sc(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
# if VTK_TYPE_CHAR_IS_SIGNED
typedef vtkInt8Type vtkCharAliasType;
#  define vtkTypeMacroCase_c(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
# endif
#else
# define vtkTypeMacroCase_sc(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_SIGNED_CHAR not compiled") 
# if VTK_TYPE_CHAR_IS_SIGNED
#  define vtkTypeMacroCase_c(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_CHAR not compiled") 
# endif
#endif

#if defined(VTK_USE_UINT8) && (VTK_SIZEOF_CHAR == 1)
typedef vtkUInt8Type vtkUnsignedCharAliasType;
# define vtkTypeMacroCase_uc(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
# if !VTK_TYPE_CHAR_IS_SIGNED
typedef vtkUInt8Type vtkCharAliasType;
#  define vtkTypeMacroCase_c(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
# endif
#else
# define vtkTypeMacroCase_uc(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_UNSIGNED_CHAR not compiled") 
# if !VTK_TYPE_CHAR_IS_SIGNED
#  define vtkTypeMacroCase_c(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_CHAR not compiled") 
# endif
#endif

#if defined(VTK_USE_INT16) && (VTK_SIZEOF_SHORT == 2)
typedef vtkInt16Type vtkShortAliasType;
# define vtkTypeMacroCase_s(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
#else
# define vtkTypeMacroCase_s(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_SHORT not compiled");
#endif

#if defined(VTK_USE_UINT16) && (VTK_SIZEOF_SHORT == 2)
typedef vtkUInt16Type vtkUnsignedShortAliasType;
# define vtkTypeMacroCase_us(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
#else
# define vtkTypeMacroCase_us(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_UNSIGNED_SHORT not compiled");
#endif

#if defined(VTK_USE_INT32) && (VTK_SIZEOF_INT == 4)
typedef vtkInt32Type vtkIntAliasType;
# define vtkTypeMacroCase_i(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
#else
# define vtkTypeMacroCase_i(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_INT not compiled");
#endif

#if defined(VTK_USE_UINT32) && (VTK_SIZEOF_INT == 4)
typedef vtkUInt32Type vtkUnsignedIntAliasType;
# define vtkTypeMacroCase_ui(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
#else
# define vtkTypeMacroCase_ui(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_UNSIGNED_INT not compiled");
#endif

#if defined(VTK_USE_INT64) && (VTK_SIZEOF_LONG == 8)
typedef vtkInt64Type vtkLongAliasType;
# define vtkTypeMacroCase_l(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
#elif defined(VTK_USE_INT32) && (VTK_SIZEOF_LONG == 4)
typedef vtkInt32Type vtkLongAliasType;
# define vtkTypeMacroCase_l(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
#else
# define vtkTypeMacroCase_l(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_LONG not compiled");
#endif

#if defined(VTK_USE_UINT64) && (VTK_SIZEOF_LONG == 8)
typedef vtkUInt64Type vtkUnsignedLongAliasType;
# define vtkTypeMacroCase_ul(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
#elif defined(VTK_USE_UINT32) && (VTK_SIZEOF_LONG == 4)
typedef vtkUInt32Type vtkUnsignedLongAliasType;
# define vtkTypeMacroCase_ul(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
#else
# define vtkTypeMacroCase_ul(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_UNSIGNED_LONG not compiled");
#endif

#if defined(VTK_USE_INT64) && (VTK_SIZEOF_ID_TYPE == 8)
typedef vtkInt64Type vtkIdTypeAliasType;
# define vtkTypeMacroCase_id(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
#elif defined(VTK_USE_INT32) && (VTK_SIZEOF_ID_TYPE == 4)
typedef vtkInt32Type vtkIdTypeAliasType;
# define vtkTypeMacroCase_id(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
#else
# define vtkTypeMacroCase_id(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_ID_TYPE not compiled");
#endif

#if defined(VTK_USE_INT64) && defined(VTK_SIZEOF_LONG_LONG)
# if (VTK_SIZEOF_LONG_LONG == 8)
typedef vtkInt64Type vtkLongLongAliasType;
#  define vtkTypeMacroCase_ll(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
# else
#  define vtkTypeMacroCase_ll(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_LONG_LONG not compiled");
# endif
#else
# define vtkTypeMacroCase_ll(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_LONG_LONG not compiled");
#endif

#if defined(VTK_USE_UINT64) && defined(VTK_SIZEOF_LONG_LONG)
# if (VTK_SIZEOF_LONG_LONG == 8)
typedef vtkUInt64Type vtkUnsignedLongLongAliasType;
#  define vtkTypeMacroCase_ull(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
# else
#  define vtkTypeMacroCase_ull(typeN, type, call)\
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_UNSIGNED_LONG_LONG not compiled");
# endif
#else
# define vtkTypeMacroCase_ull(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_ not compiled");
#endif

#if defined(VTK_USE_INT64) && defined(VTK_SIZEOF___INT64)
typedef vtkInt64Type vtk__int64AliasType;
# define vtkTypeMacroCase_i64(typeN, type, call) \
              vtkTypeMacroCase(typeN, type, call);
#else
# define vtkTypeMacroCase_i64(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK___INT64 not compiled");
#endif

#if defined(VTK_USE_UINT64) && defined(VTK_SIZEOF___INT64) && defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
typedef vtkUInt64Type vtkUnsigned__int64AliasType;
# define vtkTypeMacroCase_ui64(typeN, type, call) \
              vtkTypeMacroCase(typeN, type, call);
#else
# define vtkTypeMacroCase_ui64(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_UNSIGNED___INT64 not compiled");
#endif

#if defined(VTK_USE_FLOAT32) && (VTK_SIZEOF_FLOAT == 4)
typedef vtkFloat32Type vtkFloatAliasType;
# define vtkTypeMacroCase_f(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
#else
# define vtkTypeMacroCase_f(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_FLOAT not compiled.");
#endif

#if defined(VTK_USE_FLOAT64) && (VTK_SIZEOF_DOUBLE == 8)
typedef vtkFloat64Type vtkDoubleAliasType;
# define vtkTypeMacroCase_d(typeN, type, call) \
            vtkTypeMacroCase(typeN, type, call);
#else
# define vtkTypeMacroCase_d(typeN, type, call) \
            vtkMissingTypeWarningMacro(typeN, "Support for VTK_DOUBLE not compiled.");
#endif

#endif
