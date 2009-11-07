/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkYoungsMaterialInterface3DMath.h

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


#ifndef __vtkYoungsMaterialInterface3DMath_H
#define __vtkYoungsMaterialInterface3DMath_H

#include "vtkYoungsMaterialInterfaceCommon.h"

#ifdef DEBUG
#include <assert.h>
#endif

/*
 Computes the area of the intersection between the plane, orthognal to the 'normal' vector,
 that passes through P1 (resp. P2), and the given tetrahedron.
 the resulting area function, is a function of the intersection area given the distance of the cutting plane to the origin.
 */
FUNC_DECL
REAL tetraPlaneSurfFunc(
   const uchar4 tetra,
   const REAL3* vertices,
   const REAL3 normal,
   REAL3 func[3]
   )
{
  // 1. load the data

   const REAL3 v0 = vertices[ tetra.x ];
   const REAL3 v1 = vertices[ tetra.y ];
   const REAL3 v2 = vertices[ tetra.z ];
   const REAL3 v3 = vertices[ tetra.w ];

   const REAL d0 = dot( v0 , normal );
   const REAL d1 = dot( v1 , normal );
   const REAL d2 = dot( v2 , normal );
   const REAL d3 = dot( v3 , normal );

#ifdef DEBUG
   bool ok = (d0<=d1 && d1<=d2 && d2<=d3);
   if( !ok )
   {
      DBG_MESG( "d0="<<d0<<", d1="<<d1<<", d2="<<d2<<", d3="<<d3 );
   }
   assert( d0<=d1 && d1<=d2 && d2<=d3 );
#endif

  // 2. compute

  // surface de l'intersection en p1
   const REAL surf1 = triangleSurf(
      v1,
      linearInterp( d0, v0, d2, v2, d1 ),
      linearInterp( d0, v0, d3, v3, d1 )
      );

  // calcul de la surface d'intersection au milieu de p1 et p2
  // l'intersection est un quadrangle de sommets a,b,c,d
   const REAL d12 = (d1+d2) * REAL_CONST(0.5) ;
   const REAL3 a = linearInterp( d0, v0, d2, v2, d12);
   const REAL3 b = linearInterp( d0, v0, d3, v3, d12);
   const REAL3 c = linearInterp( d1, v1, d3, v3, d12);
   const REAL3 d = linearInterp( d1, v1, d2, v2, d12);

   const REAL surf12 = triangleSurf( a,b,d ) + triangleSurf( b,c,d );

  // surface de l'intersection en p2
   const REAL surf2 = triangleSurf(
      v2,
      linearInterp( d0, v0, d3, v3, d2 ) ,
      linearInterp( d1, v1, d3, v3, d2 ) );


  // construction des fonctions de surface
   REAL coef;

  // recherche S0(x) = coef * (x-d0)²
   coef = ( d1 > d0 )  ?  ( surf1 / ((d1-d0)*(d1-d0)) ) : REAL_CONST(0.0) ;
   func[0] = coef * make_REAL3( 1 , -2*d0 , d0*d0 ) ;

  // recherche S1(x) = interp quadric de surf1, surf12, surf2 aux points d1, d12, d2
   func[1] = quadraticInterpFunc( d1, surf1, d12, surf12, d2, surf2 );

  // de la forme S(x) = coef * (d3-x)²
   coef = ( d3 > d2 )  ?  ( surf2 / ((d3-d2)*(d3-d2)) ) : REAL_CONST(0.0) ;
   func[2] = coef * make_REAL3( 1 , -2*d3 , d3*d3 ) ;

   return tetraVolume( v0, v1, v2, v3 );
}


FUNC_DECL
REAL findTetraSetCuttingPlane(
   const REAL3 normal,    // IN  , normal vector
   const REAL fraction,   // IN  , volume fraction
   const int nv,          // IN  , number of vertices
   const int nt,          // IN  , number of tetras
   const uchar4* tv,       // IN  , tetras connectivity, size=nt
   const REAL3* vertices // IN  , vertex coordinates, size=nv
#ifdef __CUDACC__
   ,char* sdata           // TEMP Storage
#endif
   )
{
   ALLOC_LOCAL_ARRAY( rindex, unsigned char, nv );
   ALLOC_LOCAL_ARRAY( index, unsigned char, nv );
   ALLOC_LOCAL_ARRAY( derivatives, REAL3, nv-1 );

  // initialisation
   for(int i=0;i<nv;i++)
   {
      index[i] = i;
   }

  // tri des sommets dans le sens de la normale
   sortVertices( nv,  vertices, normal, index );

  // table d'indirection inverse
   for(int i=0;i<nv;i++)
   {
      rindex[ index[i] ] = i;
   }

#ifdef DEBUG
   for(int i=0;i<nv;i++)
   {
      DBG_MESG("index["<<i<<"]="<<index[i]<<", rindex["<<i<<"]="<<rindex[i]);
   }
#endif

   for(int i=0;i<(nv-1);i++)
   {
      derivatives[i] = make_REAL3(0,0,0);
   }

   REAL volume = 0;

  // construction de la fonction cubique par morceau du volume tronqué
   for(int i=0;i<nt;i++)
   {
     // calcul de la surface de l'intersection plan/tetra aux point P1 et P2
      uchar4 tetra = sortTetra( tv[i] , rindex );
      DBG_MESG( "\ntetra "<<i<<" : "<<tv[i].x<<','<<tv[i].y<<','<<tv[i].z<<','<<tv[i].w<<" -> "<<tetra.x<<','<<tetra.y<<','<<tetra.z<<','<<tetra.w );

     // calcul des sous fonctions cubiques du volume derriere le plan en fonction de la distance
      REAL3 tetraSurfFunc[3];
      volume += tetraPlaneSurfFunc( tetra, vertices, normal, tetraSurfFunc );      

#ifdef DEBUG
      for(int k=0;k<3;k++)
      {
   DBG_MESG( "surf["<<k<<"] = "<<tetraSurfFunc[k].x<<','<<tetraSurfFunc[k].y<<','<<tetraSurfFunc[k].z );
      }
#endif

     // surface function bounds
      unsigned int i0 = rindex[ tetra.x ];
      unsigned int i1 = rindex[ tetra.y ];
      unsigned int i2 = rindex[ tetra.z ];
      unsigned int i3 = rindex[ tetra.w ];

      DBG_MESG( "surf(x) steps = "<<i0<<','<<i1<<','<<i2<<','<<i3 );

      DBG_MESG( "ajout surfFunc sur ["<<i0<<';'<<i1<<"]" );
      for(unsigned int j=i0;j<i1;j++) derivatives[j] += tetraSurfFunc[0] ;

      DBG_MESG( "ajout surfFunc sur ["<<i1<<';'<<i2<<"]" );
      for(unsigned int j=i1;j<i2;j++) derivatives[j] += tetraSurfFunc[1] ;

      DBG_MESG( "ajout surfFunc sur ["<<i2<<';'<<i3<<"]" );
      for(unsigned int j=i2;j<i3;j++) derivatives[j] += tetraSurfFunc[2] ;
   }
 
  // calcul du volume recherche
   REAL y = volume*fraction;
   DBG_MESG( "volume = "<<volume<<", volume*fraction = "<<y );

  // integration des fonctions de surface en fonctions de volume
   REAL sum = 0;
   REAL4 volumeFunction = make_REAL4(0,0,0,0);
   REAL xmin ;
   REAL xmax = dot( vertices[index[0]], normal ) ;
   int s = -1;
   while( sum<y && s<(nv-2) )
   {
      xmin = xmax;
      y -= sum;
      ++ s;
      REAL4 F = integratePolynomialFunc( derivatives[s] );
      F.w = - evalPolynomialFunc( F , xmin );
      volumeFunction = F;
      xmax = dot( vertices[index[s+1]] , normal );
      sum = evalPolynomialFunc( F, xmax );
   }
   if( s<0) s=0;
  // F, F' : free derivatives

  // recherche de la portion de fonction qui contient la valeur
   DBG_MESG( "step="<<s<<", x in ["<<xmin<<';'<<xmax<<']' );

  /* chaque portion de fonction redemarre de 0,
  on calcul donc le volume recherché dans cette portion de fonction
  */
  //y -= sum;
   DBG_MESG( "volume reminder = "<< y );

  // recherche par newton
   REAL x = newtonSearchPolynomialFunc( volumeFunction, derivatives[s], y, xmin, xmax );
   
   DBG_MESG( "final x = "<< x );
   return x ;
}

#endif /* __vtkYoungsMaterialInterface3DMath_H */

