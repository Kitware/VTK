/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkYoungsMaterialInterfaceCommon.h

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

// .SECTION Caveats
// This file is for vtkYoungsMaterialInterface internal use only, it should never be included in other source files.

#ifndef __vtkYoungsMaterialInterfaceCommon_h
#define __vtkYoungsMaterialInterfaceCommon_h

#include "vtkYoungsMaterialInterfaceMacros.h" // common CUDA definitions for CUDA and host compiler

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
/**************************************
*** Precision dependant constants   ***
***************************************/

// float
#if ( REAL_PRECISION <= 32 )
#define EPSILON 1e-7
#define NEWTON_NITER 16

// long double
#elif ( REAL_PRECISION > 64 )
#define EPSILON 1e-31
#define NEWTON_NITER 64

// double ( default )
#else
#define EPSILON 1e-15 
#define NEWTON_NITER 32

#endif


/**************************************
***       Debugging                 ***
***************************************/
#define DBG_MESG(m) (void)0


/**************************************
***          Macros                 ***
***************************************/

// assure un alignement maximum des tableaux
#define ROUND_SIZE(n) (n)
//( (n+sizeof(REAL)-1) & ~(sizeof(REAL)-1) )

// local arrays allocation
#ifdef __CUDACC__

#define ALLOC_LOCAL_ARRAY(name,type,n) \
        type * name = (type*)sdata; \
        sdata += ROUND_SIZE( sizeof(type)*(n) )
#define FREE_LOCAL_ARRAY(name,type,n) sdata -= ROUND_SIZE( sizeof(type)*(n) )

#elif defined(__GNUC__) // Warning, this is a gcc extension, not all compiler accept it
#define ALLOC_LOCAL_ARRAY(name,type,n) type name[(n)]
#define FREE_LOCAL_ARRAY(name,type,n) 
#else
#include <malloc.h>
#define ALLOC_LOCAL_ARRAY(name,type,n) type* name = (type*) malloc( sizeof(type) * (n) )
#define FREE_LOCAL_ARRAY(name,type,n) free(name)
#endif

#ifdef __GNUC__
#define LOCAL_ARRAY_SIZE(n) n
#else
#define LOCAL_ARRAY_SIZE(n) 128
#endif


/*********************
 *** Triangle area ***
 *********************/
/*
 Formula from VTK in vtkTriangle.cxx, method TriangleArea
*/
FUNC_DECL
REAL triangleSurf( REAL3 p1, REAL3 p2, REAL3 p3 )
{
   const REAL3 e1 = p2-p1;
   const REAL3 e2 = p3-p2;
   const REAL3 e3 = p1-p3;

   const REAL a = dot(e1,e1);
   const REAL b = dot(e2,e2);
   const REAL c = dot(e3,e3);

   return
      REAL_CONST(0.25) *
      SQRT( FABS( 4*a*c - (a-b+c)*(a-b+c) ) )
      ;
}
FUNC_DECL
REAL triangleSurf( REAL2 p1, REAL2 p2, REAL2 p3 )
{
   const REAL2 e1 = p2-p1;
   const REAL2 e2 = p3-p2;
   const REAL2 e3 = p1-p3;

   const REAL a = dot(e1,e1);
   const REAL b = dot(e2,e2);
   const REAL c = dot(e3,e3);

   return
      REAL_CONST(0.25) *
      SQRT( FABS( 4*a*c - (a-b+c)*(a-b+c) ) )
      ;
}


/*************************
 *** Tetrahedra volume ***
 *************************/

FUNC_DECL
REAL tetraVolume( REAL3 p0, REAL3 p1, REAL3 p2, REAL3 p3 )
{
   REAL3 A = p1 - p0;
   REAL3 B = p2 - p0;
   REAL3 C = p3 - p0;
   REAL3 BC = cross(B,C);
   return FABS( dot(A,BC) / REAL_CONST(6.0) );
}

FUNC_DECL
REAL tetraVolume( const uchar4 tetra, const REAL3* vertices )
{
   return tetraVolume( vertices[tetra.x], vertices[tetra.y], vertices[tetra.z], vertices[tetra.w] );
}


/*******************************************
 *** Evaluation of a polynomial function ***
 *******************************************/
FUNC_DECL
REAL evalPolynomialFunc(const REAL2 F, const REAL x)
{
   return F.x * x + F.y ;
}

FUNC_DECL
REAL evalPolynomialFunc(const REAL3 F, const REAL x)
{
   REAL y = ( F.x * x + F.y ) * x ;
   return y + F.z;
}

FUNC_DECL
REAL evalPolynomialFunc(const REAL4 F, const REAL x)
{
   REAL y = ( ( F.x * x + F.y ) * x + F.z ) * x;
   return y + F.w; // this increases numerical stability when compiled with -ffloat-store
}


/*****************************************
 *** Intergal of a polynomial function ***
 *****************************************/
FUNC_DECL
REAL3 integratePolynomialFunc( REAL2 linearFunc )
{
   return make_REAL3( linearFunc.x/2 , linearFunc.y, 0 );
}

FUNC_DECL
REAL4 integratePolynomialFunc( REAL3 quadFunc )
{
   return make_REAL4( quadFunc.x/3, quadFunc.y/2, quadFunc.z, 0 );
}

/*******************************************
 *** Derivative of a polynomial function ***
 *******************************************/
FUNC_DECL
REAL2 derivatePolynomialFunc( REAL3 F )
{
   REAL2 dF = make_REAL2( 2*F.x, F.y );
   return dF;
}

FUNC_DECL
REAL3 derivatePolynomialFunc( REAL4 F )
{
   REAL3 dF = make_REAL3( 3*F.x, 2*F.y, F.z );
   return dF;
}

/****************************
 *** Linear interpolation ***
 ****************************/
FUNC_DECL
REAL3 linearInterp( REAL t0, REAL3 x0, REAL t1, REAL3 x1, REAL t )
{
   REAL f = (t1!=t0) ? (t-t0)/(t1-t0) : 0 ;
   return x0 + f * (x1-x0) ;
}

FUNC_DECL
REAL2 linearInterp( REAL t0, REAL2 x0, REAL t1, REAL2 x1, REAL t )
{
   REAL f = (t1!=t0) ? (t-t0)/(t1-t0) : REAL_CONST(0.0) ;
   return x0 + f * (x1-x0) ;
}

FUNC_DECL
REAL linearInterp( REAL t0, REAL x0, REAL t1, REAL x1, REAL t )
{
   REAL f = (t1!=t0) ? (t-t0)/(t1-t0) : REAL_CONST(0.0) ;
   return x0 + f * (x1-x0) ;
}


/****************************************
 *** Quadratic interpolation function ***
 ****************************************/
FUNC_DECL
REAL3 quadraticInterpFunc( REAL x0, REAL y0, REAL x1, REAL y1, REAL x2, REAL y2 )
{
  // Formula from the book 'Maillages', page 409

  // non-degenerated case (really a quadratic function)
   if( x1>x0 && x2>x1 )
   {
     // denominators
      const REAL d0 = ( x0 - x1 ) * ( x0 - x2 );
      const REAL d1 = ( x1 - x0 ) * ( x1 - x2 );
      const REAL d2 = ( x2 - x0 ) * ( x2 - x1 );

     // coefficients for the quadratic interpolation of (x0,y0) , (x1,y1) and p2(x2,y2)
      return make_REAL3(
   ( y0          / d0 ) + ( y1          / d1 ) + ( y2          / d2 ) ,  // x^2 term
   ( y0*(-x1-x2) / d0 ) + ( y1*(-x0-x2) / d1 ) + ( y2*(-x0-x1) / d2 ) ,  // x term
   ( y0*(x1*x2)  / d0 ) + ( y1*(x0*x2)  / d1 ) + ( y2*(x0*x1)  / d2 ) ); // constant term
   }

  // linear case : 2 out of the 3 points are the same
   else if( x2 > x0 )
   {
      return make_REAL3(
   0                         ,  // x^2 term
   ( y2 - y0 ) / ( x2 - x0 ) ,  // x term
   y0                        ); // constant term
   }

  // degenerated case
   else
   {
      return make_REAL3(0,0,0);
   }
}


/**************************************
 *** Analytic solver for ax²+bx+c=0 ***
 **************************************/
FUNC_DECL
REAL quadraticFunctionSolve( REAL3 F, const REAL value, const REAL xmin, const REAL xmax )
{
// resolution analytique de ax²+bx+c=0
// (!) numeriquement hazardeux, donc on prefere le newton qui est pourtant BEAUCOUP plus lent

   F.z -= value;

   REAL delta = ( F.y * F.y ) - (4 * F.x * F.z);
   REAL sqrt_delta = SQRT(delta);
   REAL x = ( -F.y - sqrt_delta ) / ( 2 * F.x );
   DBG_MESG("delta="<<delta<<", sqrt(delta)="<<sqrt_delta<<", x1="<<x<<", xmin="<<xmin<<", xmax="<<xmax);
   if( x < xmin || x > xmax ) // choose a solution inside the bounds [xmin;xmax]
   {
      x = ( -F.y + sqrt_delta ) / ( 2 * F.x );
      DBG_MESG("x2="<<x);
   }

   if( F.x == REAL_CONST(0.0) ) // < EPSILON ?
   {
      x = (F.y!=0) ? ( - F.z / F.y ) : xmin /* or nan or 0 ? */;
      DBG_MESG("xlin="<<x);
   }

   x = clamp( x , xmin , xmax ); // numerical safety
   DBG_MESG("clamp(x)="<<x);
   return x;
}

/****************************
 *** Newton search method ***
 ****************************/
FUNC_DECL
REAL newtonSearchPolynomialFunc( REAL3 F, REAL2 dF, const REAL value, const REAL xmin, const REAL xmax )
{
  // translation de F, car le newton cherche le zero de la derivee
   F.z -= value;

  // on demarre du x le plus proche entre xmin, xmilieu et xmax
   const REAL ymin = evalPolynomialFunc( F, xmin );
   const REAL ymax = evalPolynomialFunc( F, xmax );

   REAL x = ( xmin + xmax ) * REAL_CONST(0.5);
   REAL y = evalPolynomialFunc(F,x);

  // cherche x tel que F(x) = 0
#ifdef __CUDACC__
#pragma unroll
#endif
   for(int i=0;i<NEWTON_NITER;i++)
   {
      DBG_MESG("F("<<x<<")="<<y);
     // Xi+1 = Xi - F'(x)/F''(x)
      REAL d = evalPolynomialFunc(dF,x);
      if( d==0 ) { d=1; y=0; }
      x = x - ( y / d );
      y = evalPolynomialFunc(F,x);
   }

  // on verifie que la solution n'est pas moins bonne que si on prend une des deux bornes
   DBG_MESG("F("<<xmin<<")="<<ymin<<", "<<"F("<<x<<")="<<y<<", "<<"F("<<xmax<<")="<<ymax);
   y = FABS( y );
   if( FABS(ymin) < y ) { x = xmin; }
   if( FABS(ymax) < y ) { x = xmax; }

   DBG_MESG("F("<<x<<")="<<y);
   return x;
}

FUNC_DECL
REAL newtonSearchPolynomialFunc( REAL4 F,  REAL3 dF, const REAL value, const REAL xmin, const REAL xmax )
{
  // translation de F, car le newton cherche le zero de la derivee
   F.w -= value;

  // on demarre du x le plus proche entre xmin, xmilieu et xmax
   const REAL ymin = evalPolynomialFunc( F, xmin );
   const REAL ymax = evalPolynomialFunc( F, xmax );

   REAL x = ( xmin + xmax ) * REAL_CONST(0.5);
   REAL y = evalPolynomialFunc(F,x);

  // cherche x tel que F(x) = 0
#pragma unroll
   for(int i=0;i<NEWTON_NITER;i++)
   {
      DBG_MESG("F("<<x<<")="<<y);
     // Xi+1 = Xi - F'(x)/F''(x)
      REAL d = evalPolynomialFunc(dF,x);
      if( d==0 ) { d=1; y=0; }
      x = x - ( y / d );
      y = evalPolynomialFunc(F,x);
   }

  // on verifie que la solution n'est pas moins bonne que si on prend une des deux bornes
   DBG_MESG("F("<<xmin<<")="<<ymin<<", "<<"F("<<x<<")="<<y<<", "<<"F("<<xmax<<")="<<ymax);
   y = FABS( y );
   if( FABS(ymin) < y ) { x = xmin; }
   if( FABS(ymax) < y ) { x = xmax; }

   DBG_MESG("F("<<x<<")="<<y);
   return x;
}


/***********************
 *** Sorting methods ***
 ***********************/
template<typename IntType>
FUNC_DECL
void sortVertices( const int n, const REAL* dist, IntType* indices )
{
// insertion sort : slow but symetrical across all instances
#define SWAP(a,b) { IntType t = indices[a]; indices[a] = indices[b]; indices[b] = t; }
   for(int i=0;i<n;i++)
   {
      int imin = i;
      for(int j=i+1;j<n;j++)
      {
   imin = ( dist[indices[j]] < dist[indices[imin]] ) ? j : imin;
      }
      SWAP( i, imin );
   }
#undef SWAP
}

FUNC_DECL
uint3 sortTriangle( uint3 t , unsigned int* i )
{
#define SWAP(a,b) { unsigned int tmp=a; a=b; b=tmp; }
   if( i[t.y] < i[t.x] ) SWAP(t.x,t.y);
   if( i[t.z] < i[t.y] ) SWAP(t.y,t.z);
   if( i[t.y] < i[t.x] ) SWAP(t.x,t.y);
#undef SWAP
   return t;
}

FUNC_DECL
uchar3 sortTriangle( uchar3 t , unsigned char* i )
{
#define SWAP(a,b) { unsigned char tmp=a; a=b; b=tmp; }
   if( i[t.y] < i[t.x] ) SWAP(t.x,t.y);
   if( i[t.z] < i[t.y] ) SWAP(t.y,t.z);
   if( i[t.y] < i[t.x] ) SWAP(t.x,t.y);
#undef SWAP
   return t;
}

template<typename IntType>
FUNC_DECL
void sortVertices( const int n, const REAL3* vertices, const REAL3 normal, IntType* indices )
{
// insertion sort : slow but symetrical across all instances
#define SWAP(a,b) { IntType t = indices[a]; indices[a] = indices[b]; indices[b] = t; }
   for(int i=0;i<n;i++)
   {
      int imin = i;
      REAL dmin = dot(vertices[indices[i]],normal);
      for(int j=i+1;j<n;j++)
      {
   REAL d = dot(vertices[indices[j]],normal);
   imin = ( d < dmin ) ? j : imin;
   dmin = min( dmin , d );
      }
      SWAP( i, imin );
   }
#undef SWAP
}

template<typename IntType>
FUNC_DECL
void sortVertices( const int n, const REAL2* vertices, const REAL2 normal, IntType* indices )
{
// insertion sort : slow but symetrical across all instances
#define SWAP(a,b) { IntType t = indices[a]; indices[a] = indices[b]; indices[b] = t; }
   for(int i=0;i<n;i++)
   {
      int imin = i;
      REAL dmin = dot(vertices[indices[i]],normal);
      for(int j=i+1;j<n;j++)
      {
   REAL d = dot(vertices[indices[j]],normal);
   imin = ( d < dmin ) ? j : imin;
   dmin = min( dmin , d );
      }
      SWAP( i, imin );
   }
#undef SWAP
}

template<typename IntType>
FUNC_DECL
uchar4 sortTetra( uchar4 t , IntType* i )
{
#define SWAP(a,b) { IntType tmp=a; a=b; b=tmp; }
   if( i[t.y] < i[t.x] ) SWAP(t.x,t.y);
   if( i[t.w] < i[t.z] ) SWAP(t.z,t.w);
   if( i[t.z] < i[t.y] ) SWAP(t.y,t.z);
   if( i[t.y] < i[t.x] ) SWAP(t.x,t.y);
   if( i[t.w] < i[t.z] ) SWAP(t.z,t.w);
   if( i[t.z] < i[t.y] ) SWAP(t.y,t.z);
#undef SWAP
   return t;
}


#endif /* __vtkYoungsMaterialInterfaceCommon_h */


