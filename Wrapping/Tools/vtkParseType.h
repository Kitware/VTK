/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseType.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkParseType_h
#define vtkParseType_h

/**
 * The parser identifies VTK types with 32-bit hexadecimal numbers:
 *
 * - One byte is for the base type.
 * - One byte is indirection i.e. & and * and "* const"
 * - One byte is for qualifiers like const and static.
 * - The final byte is reserved.
 *
 * There is some type information that cannot be stored within
 * this bitfield.  This info falls into three categories:
 *
 * 1) Function pointers are stored in a FunctionInfo struct.
 *    However, if the type is VTK_PARSE_FUNCTION with no POINTER,
 *    it is guaranteed to be "void func(void *)" which is the
 *    old VTK-style callback.
 *
 * 2) Multi-dimensional arrays are stored in a char *[MAX_ARRAY_DIMS]
 *    array with a NULL pointer indicating there are no more brackets.
 *    If the type is a pointer and the first value is not NULL, then
 *    that value gives the array size for that pointer.  The reason
 *    that "char *" is used is because the sizes might be template
 *    parameters or constants defined elsewhere.  However, most often
 *    the sizes are integer literals, and the first size will be
 *    stored as an int in ArgCounts.
 *
 * 3) The ID for VTK_PARSE_OBJECT is stored in ArgClasses.
 *
 */

/**
 * Mask for removing everything but the base type
 */
#define VTK_PARSE_BASE_TYPE  0x000000FF

/**
 * Mask for checking signed/unsigned
 */
#define VTK_PARSE_UNSIGNED   0x00000010

/**
 * Mask for pointers and references
 */
#define VTK_PARSE_INDIRECT   0x0000FF00

/**
 * Qualifiers
 */
#define VTK_PARSE_QUALIFIER   0x00FF0000
#define VTK_PARSE_CONST       0x00010000
#define VTK_PARSE_STATIC      0x00020000
#define VTK_PARSE_VIRTUAL     0x00040000
#define VTK_PARSE_EXPLICIT    0x00080000
#define VTK_PARSE_MUTABLE     0x00100000
#define VTK_PARSE_VOLATILE    0x00200000
#define VTK_PARSE_RVALUE      0x00400000
#define VTK_PARSE_THREAD_LOCAL 0x00800000

/**
 * Attributes (used for hints)
 */
#define VTK_PARSE_ATTRIBUTES  0x03000000
#define VTK_PARSE_NEWINSTANCE 0x01000000
#define VTK_PARSE_ZEROCOPY    0x02000000

/**
 * Special
 */
#define VTK_PARSE_SPECIALS    0x70000000
#define VTK_PARSE_TYPEDEF     0x10000000
#define VTK_PARSE_FRIEND      0x20000000
#define VTK_PARSE_PACK        0x40000000

/**
 * Mask for removing qualifiers
 */
#define VTK_PARSE_QUALIFIED_TYPE   0x03FFFFFF
#define VTK_PARSE_UNQUALIFIED_TYPE 0x0000FFFF

/**
 * Indirection, contained in VTK_PARSE_INDIRECT
 *
 * Indirection of types works as follows:
 * type **(**&val[n])[m]
 * Pointers on the left, arrays on the right,
 * and optionally a set of parentheses and a ref.
 *
 * The 'type' may be preceded or followed by const,
 * which is handled by the VTK_PARSE_CONST flag.
 *
 * The array dimensionality and sizes is stored
 * elsewhere, it isn't stored in the bitfield.
 *
 * The leftmost [] is converted to a pointer, unless
 * it is outside the parenthesis.
 * So "type val[n][m]"  becomes  "type (*val)[m]",
 * these two types are identical in C and C++.
 *
 * Any pointer can be followed by const, and any pointer
 * can be preceded by a parenthesis. However, you will
 * never see a parenthesis anywhere except for just before
 * the leftmost pointer.
 *
 * These are good: "(*val)[n]", "**(*val)[n]", "(*&val)[n]"
 * Not so good: "(**val)[n]" (is actually like (*val)[][n])
 *
 * The Ref needs 1 bit total, and each pointer needs 2 bits:
 *
 *  0 = nothing
 *  1 = '*'       = VTK_PARSE_POINTER
 *  2 = '[]'      = VTK_PARSE_ARRAY
 *  3 = '* const' = VTK_PARSE_CONST_POINTER
 *
 * The VTK_PARSE_ARRAY flag means "this pointer is actually
 * the first bracket in a multi-dimensional array" with the array
 * info stored separately.
 */
#define VTK_PARSE_BAD_INDIRECT          0xFF00
#define VTK_PARSE_POINTER_MASK          0xFE00
#define VTK_PARSE_POINTER_LOWMASK       0x0600
#define VTK_PARSE_REF                   0x0100
#define VTK_PARSE_POINTER               0x0200
#define VTK_PARSE_POINTER_REF           0x0300
#define VTK_PARSE_ARRAY                 0x0400
#define VTK_PARSE_ARRAY_REF             0x0500
#define VTK_PARSE_CONST_POINTER         0x0600
#define VTK_PARSE_CONST_POINTER_REF     0x0700
#define VTK_PARSE_POINTER_POINTER       0x0A00
#define VTK_PARSE_POINTER_POINTER_REF   0x0B00
#define VTK_PARSE_POINTER_CONST_POINTER 0x0E00

/**
 * Basic types contained in VTK_PARSE_BASE_TYPE
 *
 * The lowest two hex digits describe the basic type,
 * where bit 0x10 is used to indicate unsigned types,
 * value 0x8 is used for unrecognized types, and
 * value 0x9 is used for types that start with "vtk".
 *
 * The bit 0x10 is reserved for "unsigned", and it
 * may only be present in unsigned types.
 *
 * Do not rearrange these types, they are hard-coded
 * into the hints file.
 */
#define VTK_PARSE_FLOAT               0x01
#define VTK_PARSE_VOID                0x02
#define VTK_PARSE_CHAR                0x03
#define VTK_PARSE_UNSIGNED_CHAR       0x13
#define VTK_PARSE_INT                 0x04
#define VTK_PARSE_UNSIGNED_INT        0x14
#define VTK_PARSE_SHORT               0x05
#define VTK_PARSE_UNSIGNED_SHORT      0x15
#define VTK_PARSE_LONG                0x06
#define VTK_PARSE_UNSIGNED_LONG       0x16
#define VTK_PARSE_DOUBLE              0x07
#define VTK_PARSE_UNKNOWN             0x08
#define VTK_PARSE_OBJECT              0x09
#define VTK_PARSE_ID_TYPE             0x0A
#define VTK_PARSE_UNSIGNED_ID_TYPE    0x1A
#define VTK_PARSE_LONG_LONG           0x0B
#define VTK_PARSE_UNSIGNED_LONG_LONG  0x1B
#define VTK_PARSE___INT64             0x0C
#define VTK_PARSE_UNSIGNED___INT64    0x1C
#define VTK_PARSE_SIGNED_CHAR         0x0D
#define VTK_PARSE_BOOL                0x0E
#define VTK_PARSE_SSIZE_T             0x0F
#define VTK_PARSE_SIZE_T              0x1F
#define VTK_PARSE_STRING              0x21
#define VTK_PARSE_UNICODE_STRING      0x22
#define VTK_PARSE_OSTREAM             0x23
#define VTK_PARSE_ISTREAM             0x24
#define VTK_PARSE_FUNCTION            0x25
#define VTK_PARSE_QOBJECT             0x26
#define VTK_PARSE_LONG_DOUBLE         0x27
#define VTK_PARSE_WCHAR_T             0x28
#define VTK_PARSE_CHAR16_T            0x29
#define VTK_PARSE_CHAR32_T            0x2A
#define VTK_PARSE_NULLPTR_T           0x2B

/**
 * Basic pointer types
 */
#define VTK_PARSE_FLOAT_PTR               0x201
#define VTK_PARSE_VOID_PTR                0x202
#define VTK_PARSE_CHAR_PTR                0x203
#define VTK_PARSE_UNSIGNED_CHAR_PTR       0x213
#define VTK_PARSE_INT_PTR                 0x204
#define VTK_PARSE_UNSIGNED_INT_PTR        0x214
#define VTK_PARSE_SHORT_PTR               0x205
#define VTK_PARSE_UNSIGNED_SHORT_PTR      0x215
#define VTK_PARSE_LONG_PTR                0x206
#define VTK_PARSE_UNSIGNED_LONG_PTR       0x216
#define VTK_PARSE_DOUBLE_PTR              0x207
#define VTK_PARSE_UNKNOWN_PTR             0x208
#define VTK_PARSE_OBJECT_PTR              0x209
#define VTK_PARSE_ID_TYPE_PTR             0x20A
#define VTK_PARSE_UNSIGNED_ID_TYPE_PTR    0x21A
#define VTK_PARSE_LONG_LONG_PTR           0x20B
#define VTK_PARSE_UNSIGNED_LONG_LONG_PTR  0x21B
#define VTK_PARSE___INT64_PTR             0x20C
#define VTK_PARSE_UNSIGNED___INT64_PTR    0x21C
#define VTK_PARSE_SIGNED_CHAR_PTR         0x20D
#define VTK_PARSE_BOOL_PTR                0x20E
#define VTK_PARSE_SSIZE_T_PTR             0x20F
#define VTK_PARSE_SIZE_T_PTR              0x21F
#define VTK_PARSE_STRING_PTR              0x221
#define VTK_PARSE_UNICODE_STRING_PTR      0x222
#define VTK_PARSE_OSTREAM_PTR             0x223
#define VTK_PARSE_ISTREAM_PTR             0x224
#define VTK_PARSE_FUNCTION_PTR            0x225
#define VTK_PARSE_QOBJECT_PTR             0x226
#define VTK_PARSE_LONG_DOUBLE_PTR         0x227
#define VTK_PARSE_WCHAR_T_PTR             0x228
#define VTK_PARSE_CHAR16_T_PTR            0x229
#define VTK_PARSE_CHAR32_T_PTR            0x22A
#define VTK_PARSE_NULLPTR_T_PTR           0x22B


/**
 * Basic reference types
 */
#define VTK_PARSE_FLOAT_REF               0x101
#define VTK_PARSE_VOID_REF                0x102
#define VTK_PARSE_CHAR_REF                0x103
#define VTK_PARSE_UNSIGNED_CHAR_REF       0x113
#define VTK_PARSE_INT_REF                 0x104
#define VTK_PARSE_UNSIGNED_INT_REF        0x114
#define VTK_PARSE_SHORT_REF               0x105
#define VTK_PARSE_UNSIGNED_SHORT_REF      0x115
#define VTK_PARSE_LONG_REF                0x106
#define VTK_PARSE_UNSIGNED_LONG_REF       0x116
#define VTK_PARSE_DOUBLE_REF              0x107
#define VTK_PARSE_UNKNOWN_REF             0x108
#define VTK_PARSE_OBJECT_REF              0x109
#define VTK_PARSE_ID_TYPE_REF             0x10A
#define VTK_PARSE_UNSIGNED_ID_TYPE_REF    0x11A
#define VTK_PARSE_LONG_LONG_REF           0x10B
#define VTK_PARSE_UNSIGNED_LONG_LONG_REF  0x11B
#define VTK_PARSE___INT64_REF             0x10C
#define VTK_PARSE_UNSIGNED___INT64_REF    0x11C
#define VTK_PARSE_SIGNED_CHAR_REF         0x10D
#define VTK_PARSE_BOOL_REF                0x10E
#define VTK_PARSE_SSIZE_T_REF             0x10F
#define VTK_PARSE_SIZE_T_REF              0x11F
#define VTK_PARSE_STRING_REF              0x121
#define VTK_PARSE_UNICODE_STRING_REF      0x122
#define VTK_PARSE_OSTREAM_REF             0x123
#define VTK_PARSE_ISTREAM_REF             0x124
#define VTK_PARSE_QOBJECT_REF             0x126
#define VTK_PARSE_LONG_DOUBLE_REF         0x127
#define VTK_PARSE_WCHAR_T_REF             0x128
#define VTK_PARSE_CHAR16_T_REF            0x129
#define VTK_PARSE_CHAR32_T_REF            0x12A
#define VTK_PARSE_NULLPTR_T_REF           0x12B

/**
 * For backwards compatibility
 */
#ifndef VTK_PARSE_LEGACY_REMOVE
#define VTK_PARSE_VTK_OBJECT        VTK_PARSE_OBJECT
#define VTK_PARSE_VTK_OBJECT_PTR    VTK_PARSE_OBJECT_PTR
#define VTK_PARSE_VTK_OBJECT_REF    VTK_PARSE_OBJECT_REF
#endif

#endif
