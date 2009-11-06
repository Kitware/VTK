/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkYoungsMaterialInterfaceVecMath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .SECTION Thanks
// <verbatim>
//
// This file is part of the generalized Youngs material interface reconstruction algorithm contributed by
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France
// BP12, F-91297 Arpajon, France
// 
// Implementation by Thierry Carrard (CEA)
//
// </verbatim>

/*
 Some of the vector functions where found in the file vector_operators.h from the NVIDIA's CUDA Toolkit.
 Please read the above notice.
*/

/*
 * Copyright 1993-2007 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO USER:   
 *
 * This source code is subject to NVIDIA ownership rights under U.S. and 
 * international Copyright laws.  Users and possessors of this source code 
 * are hereby granted a nonexclusive, royalty-free license to use this code 
 * in individual and commercial software.
 *
 * NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE 
 * CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR 
 * IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH 
 * REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF 
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL, 
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS 
 * OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE 
 * OR PERFORMANCE OF THIS SOURCE CODE.  
 *
 * U.S. Government End Users.   This source code is a "commercial item" as 
 * that term is defined at  48 C.F.R. 2.101 (OCT 1995), consisting  of 
 * "commercial computer  software"  and "commercial computer software 
 * documentation" as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995) 
 * and is provided to the U.S. Government only as a commercial end item.  
 * Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through 
 * 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the 
 * source code with only those rights set forth herein. 
 *
 * Any use of this source code in individual and commercial software must 
 * include, in the user documentation and internal comments to the code,
 * the above Disclaimer and U.S. Government End Users Notice.
 */


#ifndef __vtkYoungsMaterialInterfaceVecMath_h
#define __vtkYoungsMaterialInterfaceVecMath_h

// define base vector types and operators or use those provided by CUDA
#ifndef __CUDACC__
struct float2 { float x,y; };
struct float3 { float x,y,z; };
struct float4 { float x,y,z,w; };
struct double2 { double x,y; };
struct uint3 {unsigned int x,y,z; };
struct uint4 {unsigned int x,y,z,w; };
struct uchar4 {unsigned char x,y,z,w; };
struct uchar3 {unsigned char x,y,z; };
FUNC_DECL float2 make_float2(float x,float y)
{
   float2 v = {x,y};
   return v;
}
FUNC_DECL float3 make_float3(float x,float y,float z)
{
   float3 v = {x,y,z};
   return v;
}
FUNC_DECL float4 make_float4(float x,float y,float z,float w)
{
   float4 v = {x,y,z,w};
   return v;
}
template<typename T> static inline T min(T a, T b){ return (a<b)?a:b; }
template<typename T> static inline T max(T a, T b){ return (a>b)?a:b; }
#else
#include <vector_types.h>
#include <vector_functions.h>
#endif

#include <math.h>

#ifndef FUNC_DECL
#define FUNC_DECL static inline
#endif

FUNC_DECL  float3 operator *(float3 a, float3 b)
{
    return make_float3(a.x*b.x, a.y*b.y, a.z*b.z);
}

FUNC_DECL float3 operator *(float f, float3 v)
{
    return make_float3(v.x*f, v.y*f, v.z*f);
}

FUNC_DECL float2 operator *(float f, float2 v)
{
    return make_float2(v.x*f, v.y*f);
}

FUNC_DECL float3 operator *(float3 v, float f)
{
    return make_float3(v.x*f, v.y*f, v.z*f);
}

FUNC_DECL float2 operator *(float2 v,float f)
{
    return make_float2(v.x*f, v.y*f);
}

FUNC_DECL float4 operator *(float4 v, float f)
{
    return make_float4(v.x*f, v.y*f, v.z*f, v.w*f);
}
FUNC_DECL float4 operator *(float f, float4 v)
{
    return make_float4(v.x*f, v.y*f, v.z*f, v.w*f);
}


FUNC_DECL float2 operator +(float2 a, float2 b)
{
    return make_float2(a.x+b.x, a.y+b.y);
}


FUNC_DECL float3 operator +(float3 a, float3 b)
{
    return make_float3(a.x+b.x, a.y+b.y, a.z+b.z);
}

FUNC_DECL void operator +=(float3 & b, float3 a)
{
    b.x += a.x;
    b.y += a.y;
    b.z += a.z;
}
FUNC_DECL void operator +=(float2 & b, float2 a)
{
    b.x += a.x;
    b.y += a.y;
}


FUNC_DECL void operator +=(float4 & b, float4 a)
{
    b.x += a.x;
    b.y += a.y;
    b.z += a.z;
    b.w += a.w;
}

FUNC_DECL float3 operator -(float3 a, float3 b)
{
    return make_float3(a.x-b.x, a.y-b.y, a.z-b.z);
}

FUNC_DECL float2 operator -(float2 a, float2 b)
{
    return make_float2(a.x-b.x, a.y-b.y);
}

FUNC_DECL void operator -=(float3 & b, float3 a)
{
    b.x -= a.x;
    b.y -= a.y;
    b.z -= a.z;
}

FUNC_DECL float3 operator /(float3 v, float f)
{
    float inv = 1.0f / f;
    return v * inv;
}

FUNC_DECL void operator /=(float2 & b, float f)
{
    float inv = 1.0f / f;
    b.x *= inv;
    b.y *= inv;
}

FUNC_DECL void operator /=(float3 & b, float f)
{
    float inv = 1.0f / f;
    b.x *= inv;
    b.y *= inv;
    b.z *= inv;
}

FUNC_DECL float dot(float2 a, float2 b)
{
    return a.x * b.x + a.y * b.y;
}

FUNC_DECL float dot(float3 a, float3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

FUNC_DECL float dot(float4 a, float4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

FUNC_DECL float clamp(float f, float a, float b)
{
    return max(a, min(f, b));
}

FUNC_DECL float3 clamp(float3 v, float a, float b)
{
    return make_float3(clamp(v.x, a, b), clamp(v.y, a, b), clamp(v.z, a, b));
}

FUNC_DECL float3 clamp(float3 v, float3 a, float3 b)
{
    return make_float3(clamp(v.x, a.x, b.x), clamp(v.y, a.y, b.y), clamp(v.z, a.z, b.z));
}

FUNC_DECL float2 normalize(float2 v)
{
    float len = 1.0f / sqrtf(dot(v, v));
    return make_float2(v.x * len, v.y * len);
}

FUNC_DECL float3 normalize(float3 v)
{
    float len = 1.0f / sqrtf(dot(v, v));
    return make_float3(v.x * len, v.y * len, v.z * len);
}

FUNC_DECL float3 cross( float3 A, float3 B)
{
   return make_float3( A.y * B.z - A.z * B.y ,
          A.z * B.x - A.x * B.z ,
          A.x * B.y - A.y * B.x );
}



#ifndef __CUDACC__

/* -------------------------------------------------------- */
/* ----------- DOUBLE ------------------------------------- */
/* -------------------------------------------------------- */

struct double3 { double x,y,z; };
struct double4 { double x,y,z,w; };

FUNC_DECL double2 make_double2(double x,double y)
{
   double2 v = {x,y};
   return v;
}

FUNC_DECL double3 make_double3(double x,double y,double z)
{
   double3 v = {x,y,z};
   return v;
}

FUNC_DECL double4 make_double4(double x,double y,double z,double w)
{
   double4 v = {x,y,z,w};
   return v;
}

FUNC_DECL  double3 operator *(double3 a, double3 b)
{
    return make_double3(a.x*b.x, a.y*b.y, a.z*b.z);
}

FUNC_DECL double3 operator *(double f, double3 v)
{
    return make_double3(v.x*f, v.y*f, v.z*f);
}

FUNC_DECL double3 operator *(double3 v, double f)
{
    return make_double3(v.x*f, v.y*f, v.z*f);
}

FUNC_DECL double2 operator *(double2 v, double f)
{
    return make_double2(v.x*f, v.y*f);
}

FUNC_DECL double2 operator *(double f, double2 v)
{
    return make_double2(v.x*f, v.y*f);
}

FUNC_DECL double4 operator *(double4 v, double f)
{
    return make_double4(v.x*f, v.y*f, v.z*f, v.w*f);
}
FUNC_DECL double4 operator *(double f, double4 v)
{
    return make_double4(v.x*f, v.y*f, v.z*f, v.w*f);
}


FUNC_DECL double3 operator +(double3 a, double3 b)
{
    return make_double3(a.x+b.x, a.y+b.y, a.z+b.z);
}

FUNC_DECL double2 operator +(double2 a, double2 b)
{
    return make_double2(a.x+b.x, a.y+b.y);
}

FUNC_DECL void operator +=(double3 & b, double3 a)
{
    b.x += a.x;
    b.y += a.y;
    b.z += a.z;
}
FUNC_DECL void operator +=(double2 & b, double2 a)
{
    b.x += a.x;
    b.y += a.y;
}


FUNC_DECL void operator +=(double4 & b, double4 a)
{
    b.x += a.x;
    b.y += a.y;
    b.z += a.z;
    b.w += a.w;
}

FUNC_DECL double3 operator - (double3 a, double3 b)
{
    return make_double3(a.x-b.x, a.y-b.y, a.z-b.z);
}

FUNC_DECL double2 operator - (double2 a, double2 b)
{
    return make_double2(a.x-b.x, a.y-b.y);
}

FUNC_DECL void operator -= (double3 & b, double3 a)
{
    b.x -= a.x;
    b.y -= a.y;
    b.z -= a.z;
}

FUNC_DECL double3 operator / (double3 v, double f)
{
   return make_double3( v.x/f, v.y/f, v.z/f );
}

FUNC_DECL void operator /=(double2 & b, double f)
{
    b.x /= f;
    b.y /= f;
}

FUNC_DECL void operator /=(double3 & b, double f)
{
    b.x /= f;
    b.y /= f;
    b.z /= f;
}

FUNC_DECL double dot(double2 a, double2 b)
{
    return a.x * b.x + a.y * b.y ;
}

FUNC_DECL double dot(double3 a, double3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

FUNC_DECL double dot(double4 a, double4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

FUNC_DECL double clamp(double f, double a, double b)
{
    return max(a, min(f, b));
}

FUNC_DECL double3 clamp(double3 v, double a, double b)
{
    return make_double3(clamp(v.x, a, b), clamp(v.y, a, b), clamp(v.z, a, b));
}

FUNC_DECL double3 clamp(double3 v, double3 a, double3 b)
{
    return make_double3(clamp(v.x, a.x, b.x), clamp(v.y, a.y, b.y), clamp(v.z, a.z, b.z));
}

FUNC_DECL double3 normalize(double3 v)
{
    double len = sqrt(dot(v, v));
    return make_double3(v.x / len, v.y / len, v.z / len);
}

FUNC_DECL double2 normalize(double2 v)
{
    double len = sqrt( dot(v,v) );
    return make_double2(v.x / len, v.y / len);
}

FUNC_DECL double3 cross( double3 A, double3 B)
{
   return make_double3( A.y * B.z - A.z * B.y ,
      A.z * B.x - A.x * B.z ,
      A.x * B.y - A.y * B.x );
}


/* -------------------------------------------------------- */
/* ----------- LONG DOUBLE -------------------------------- */
/* -------------------------------------------------------- */

struct ldouble2 { long double x,y; };
struct ldouble3 { long double x,y,z; };
struct ldouble4 { long double x,y,z,w; };

FUNC_DECL ldouble2 make_ldouble2(long double x,long double y)
{
   ldouble2 v = {x,y};
   return v;
}


FUNC_DECL ldouble3 make_ldouble3(long double x,long double y,long double z)
{
   ldouble3 v = {x,y,z};
   return v;
}

FUNC_DECL ldouble4 make_ldouble4(long double x,long double y,long double z,long double w)
{
   ldouble4 v = {x,y,z,w};
   return v;
}

FUNC_DECL  ldouble3 operator *(ldouble3 a, ldouble3 b)
{
    return make_ldouble3(a.x*b.x, a.y*b.y, a.z*b.z);
}

FUNC_DECL ldouble2 operator * (long double f, ldouble2 v)
{
    return make_ldouble2(v.x*f, v.y*f);
}

FUNC_DECL ldouble3 operator *(long double f, ldouble3 v)
{
    return make_ldouble3(v.x*f, v.y*f, v.z*f);
}

FUNC_DECL ldouble2 operator * (ldouble2 v, long double f)
{
    return make_ldouble2(v.x*f, v.y*f);
}

FUNC_DECL ldouble3 operator *(ldouble3 v, long double f)
{
    return make_ldouble3(v.x*f, v.y*f, v.z*f);
}

FUNC_DECL ldouble4 operator *(ldouble4 v, long double f)
{
    return make_ldouble4(v.x*f, v.y*f, v.z*f, v.w*f);
}
FUNC_DECL ldouble4 operator *(long double f, ldouble4 v)
{
    return make_ldouble4(v.x*f, v.y*f, v.z*f, v.w*f);
}


FUNC_DECL ldouble2 operator +(ldouble2 a, ldouble2 b)
{
    return make_ldouble2(a.x+b.x, a.y+b.y);
}

FUNC_DECL ldouble3 operator +(ldouble3 a, ldouble3 b)
{
    return make_ldouble3(a.x+b.x, a.y+b.y, a.z+b.z);
}

FUNC_DECL void operator += (ldouble3 & b, ldouble3 a)
{
    b.x += a.x;
    b.y += a.y;
    b.z += a.z;
}

FUNC_DECL void operator += (ldouble2 & b, ldouble2 a)
{
    b.x += a.x;
    b.y += a.y;
}


FUNC_DECL void operator += (ldouble4 & b, ldouble4 a)
{
    b.x += a.x;
    b.y += a.y;
    b.z += a.z;
    b.w += a.w;
}

FUNC_DECL ldouble2 operator - (ldouble2 a, ldouble2 b)
{
    return make_ldouble2(a.x-b.x, a.y-b.y);
}

FUNC_DECL ldouble3 operator - (ldouble3 a, ldouble3 b)
{
    return make_ldouble3(a.x-b.x, a.y-b.y, a.z-b.z);
}

FUNC_DECL void operator -= (ldouble3 & b, ldouble3 a)
{
    b.x -= a.x;
    b.y -= a.y;
    b.z -= a.z;
}

FUNC_DECL ldouble3 operator / (ldouble3 v, long double f)
{
    return make_ldouble3( v.x/f, v.y/f, v.z/f );
}

FUNC_DECL void operator /= (ldouble3 & b, long double f)
{
    b.x /= f;
    b.y /= f;
    b.z /= f;
}

FUNC_DECL long double dot(ldouble2 a, ldouble2 b)
{
    return a.x * b.x + a.y * b.y ;
}

FUNC_DECL long double dot(ldouble3 a, ldouble3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

FUNC_DECL long double dot(ldouble4 a, ldouble4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

FUNC_DECL long double clamp(long double f, long double a, long double b)
{
    return max(a, min(f, b));
}

FUNC_DECL ldouble3 clamp(ldouble3 v, long double a, long double b)
{
    return make_ldouble3(clamp(v.x, a, b), clamp(v.y, a, b), clamp(v.z, a, b));
}

FUNC_DECL ldouble3 clamp(ldouble3 v, ldouble3 a, ldouble3 b)
{
    return make_ldouble3(clamp(v.x, a.x, b.x), clamp(v.y, a.y, b.y), clamp(v.z, a.z, b.z));
}

FUNC_DECL ldouble2 normalize(ldouble2 v)
{
    long double len = sqrtl( dot(v,v) );
    return make_ldouble2(v.x / len, v.y / len);
}

FUNC_DECL ldouble3 normalize(ldouble3 v)
{
    long double len = sqrtl( dot(v,v) );
    return make_ldouble3(v.x / len, v.y / len, v.z / len);
}

FUNC_DECL ldouble3 cross( ldouble3 A, ldouble3 B)
{
   return make_ldouble3( A.y * B.z - A.z * B.y ,
       A.z * B.x - A.x * B.z ,
       A.x * B.y - A.y * B.x );
}


#endif /* __CUDACC__ */

#endif /* __vtkYoungsMaterialInterfaceVecMath_h */

