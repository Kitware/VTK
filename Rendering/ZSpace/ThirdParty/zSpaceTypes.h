//////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2007-2015 zSpace, Inc.  All Rights Reserved.
//
//  File:       zSpaceTypes.h
//  Content:    Shared zSpace types.
//  SVN Info:   $Id$
//
//////////////////////////////////////////////////////////////////////////

#ifndef __ZSPACE_TYPES_H__
#define __ZSPACE_TYPES_H__


/// @defgroup Common Common
/// @{


//////////////////////////////////////////////////////////////////////////
// Basic Types
//////////////////////////////////////////////////////////////////////////

#if (defined(_MSC_VER) && (_MSC_VER >= 1300))

typedef signed __int8       ZSInt8;
typedef signed __int16      ZSInt16;
typedef signed __int32      ZSInt32;
typedef signed __int64      ZSInt64;

typedef unsigned __int8     ZSUInt8;
typedef unsigned __int16    ZSUInt16;
typedef unsigned __int32    ZSUInt32;
typedef unsigned __int64    ZSUInt64;

#else

// From ISO/IEC 988:1999 spec
// 7.18.1.1 Exact-width integer types
typedef signed char         ZSInt8;
typedef short               ZSInt16;
typedef int                 ZSInt32;
typedef long long           ZSInt64;

typedef unsigned char       ZSUInt8;
typedef unsigned short      ZSUInt16;
typedef unsigned int        ZSUInt32;
typedef unsigned long long  ZSUInt64;

#endif

typedef ZSInt8              ZSBool;
typedef float               ZSFloat;
typedef double              ZSDouble;


//////////////////////////////////////////////////////////////////////////
// Compound Types
//////////////////////////////////////////////////////////////////////////

// Ensure 8 byte packing.
#pragma pack( push, 8 )

/// @brief Union representing a vector of 3 floats.
typedef union ZSVector3
{
    ZSFloat f[3];
    struct
    {
        ZSFloat x;
        ZSFloat y;
        ZSFloat z;
    };
} ZSVector3;


/// @brief Union representing 4x4 matrix (right-handed OpenGL column-major format).
////
/// This structure is used by both the Stereo Frustum and Coordinate Space APIs.
typedef union ZSMatrix4
{
    ZSFloat f[16];
    struct
    {
        ZSFloat m00, m10, m20, m30;
        ZSFloat m01, m11, m21, m31;
        ZSFloat m02, m12, m22, m32;
        ZSFloat m03, m13, m23, m33;
    };
} ZSMatrix4;

#pragma pack( pop )


/// @}


#endif // __ZSPACE_TYPES_H__
