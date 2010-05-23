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

#ifndef VTK_PARSE_TYPE_H
#define VTK_PARSE_TYPE_H

/*
 * The parser identifies VTK types with 16-bit hexidecimal numbers:
 *
 * - The final two digits are the base type.
 * - The second digit is for indirection i.e. * and &
 * - The first digit is for const and static qualifiers
 * - The special value 0x5000 is for function pointers
*/

/*
 * Mask for removing "const", "static" qualifiers
 */

#define VTK_PARSE_UNQUALIFIED_TYPE 0x0FFF

/*
 * Mask for removing everything but the base type
 */

#define VTK_PARSE_BASE_TYPE  0x00FF

/*
 * Mask for checking signed/unsigned
 */

#define VTK_PARSE_UNSIGNED   0x0010

/*
 * Special function pointer type
 */

#define VTK_PARSE_FUNCTION  0x5000

/*
 * Storage qualifiers: static and const
 */

#define VTK_PARSE_QUALIFIER   0x3000
#define VTK_PARSE_CONST       0x1000
#define VTK_PARSE_STATIC      0x2000
#define VTK_PARSE_STATIC_CONST 0x3000

/*
 * Pointers, arrays, and references
 * (note that []  and * are equivalent)
 */

#define VTK_PARSE_INDIRECT              0xF00
#define VTK_PARSE_REF                   0x100
#define VTK_PARSE_CONST_POINTER         0x200
#define VTK_PARSE_POINTER               0x300
#define VTK_PARSE_CONST_POINTER_REF     0x400
#define VTK_PARSE_POINTER_REF           0x500
#define VTK_PARSE_ARRAY_2D              0x600
#define VTK_PARSE_POINTER_POINTER       0x700
#define VTK_PARSE_POINTER_CONST_POINTER 0x800
#define VTK_PARSE_ARRAY_3D              0x900
#define VTK_PARSE_BAD_INDIRECT          0xF00

/*
 * The lowest two hex digits describe the basic type,
 * where bit 0x10 is used to indicate unsigned types,
 * value 0x8 is used for unrecognized types, and
 * value 0x9 is used for all VTK objects.
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
#define VTK_PARSE_VTK_OBJECT          0x09
#define VTK_PARSE_ID_TYPE             0x0A
#define VTK_PARSE_UNSIGNED_ID_TYPE    0x1A
#define VTK_PARSE_LONG_LONG           0x0B
#define VTK_PARSE_UNSIGNED_LONG_LONG  0x1B
#define VTK_PARSE___INT64             0x0C
#define VTK_PARSE_UNSIGNED___INT64    0x1C
#define VTK_PARSE_SIGNED_CHAR         0x0D
#define VTK_PARSE_BOOL                0x0E
#define VTK_PARSE_STRING              0x21
#define VTK_PARSE_UNICODE_STRING      0x22

/*
 * Basic pointer types
 */

#define VTK_PARSE_FLOAT_PTR               0x301
#define VTK_PARSE_VOID_PTR                0x302
#define VTK_PARSE_CHAR_PTR                0x303
#define VTK_PARSE_UNSIGNED_CHAR_PTR       0x313
#define VTK_PARSE_INT_PTR                 0x304
#define VTK_PARSE_UNSIGNED_INT_PTR        0x314
#define VTK_PARSE_SHORT_PTR               0x305
#define VTK_PARSE_UNSIGNED_SHORT_PTR      0x315
#define VTK_PARSE_LONG_PTR                0x306
#define VTK_PARSE_UNSIGNED_LONG_PTR       0x316
#define VTK_PARSE_DOUBLE_PTR              0x307
#define VTK_PARSE_UNKNOWN_PTR             0x308
#define VTK_PARSE_VTK_OBJECT_PTR          0x309
#define VTK_PARSE_ID_TYPE_PTR             0x30A
#define VTK_PARSE_UNSIGNED_ID_TYPE_PTR    0x31A
#define VTK_PARSE_LONG_LONG_PTR           0x30B
#define VTK_PARSE_UNSIGNED_LONG_LONG_PTR  0x31B
#define VTK_PARSE___INT64_PTR             0x30C
#define VTK_PARSE_UNSIGNED___INT64_PTR    0x31C
#define VTK_PARSE_SIGNED_CHAR_PTR         0x30D
#define VTK_PARSE_BOOL_PTR                0x30E
#define VTK_PARSE_STRING_PTR              0x321
#define VTK_PARSE_UNICODE_STRING_PTR      0x322

/*
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
#define VTK_PARSE_VTK_OBJECT_REF          0x109
#define VTK_PARSE_ID_TYPE_REF             0x10A
#define VTK_PARSE_UNSIGNED_ID_TYPE_REF    0x11A
#define VTK_PARSE_LONG_LONG_REF           0x10B
#define VTK_PARSE_UNSIGNED_LONG_LONG_REF  0x11B
#define VTK_PARSE___INT64_REF             0x10C
#define VTK_PARSE_UNSIGNED___INT64_REF    0x11C
#define VTK_PARSE_SIGNED_CHAR_REF         0x10D
#define VTK_PARSE_BOOL_REF                0x10E
#define VTK_PARSE_STRING_REF              0x121
#define VTK_PARSE_UNICODE_STRING_REF      0x122

#endif
