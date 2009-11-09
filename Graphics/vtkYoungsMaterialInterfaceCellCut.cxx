/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkYoungsMaterialInterfaceCellCut.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This file is part of the generalized Youngs material interface reconstruction algorithm contributed by
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
// BP12, F-91297 Arpajon, France. <br>
// Implementation by Thierry Carrard (CEA)

#include "vtkYoungsMaterialInterfaceCellCut.h"

#include <vtkstd/vector>
#include <vtkstd/map>
#include <vtkstd/algorithm>

#include <math.h>

// here after the low-level functions that compute placement of the interface given a normal vector and a set of simplices
namespace vtkYoungsMaterialInterfaceCellCutInternals
{
#define REAL_PRECISION 64 // use double precision
#define REAL_COORD REAL3

#include "vtkYoungsMaterialInterfaceCommon.h"
#include "vtkYoungsMaterialInterface2DMath.h"
#include "vtkYoungsMaterialInterface2DAxisMath.h"
#include "vtkYoungsMaterialInterface3DMath.h"

typedef REAL Real;
typedef REAL2 Real2;
typedef REAL3 Real3;
typedef REAL4 Real4;

#undef REAL_PRECISION 
#undef REAL_COORD
}

// usefull to avoid numerical errors
#define Clamp(x,min,max) if(x<min) x=min; else if(x>max) x=max

struct VertexInfo
{
      double coord[3];
      double weight;
      int eid[2];
};

struct CWVertex
{
      double angle;
      double coord[3];
      double weight;
      int eid[2];
      inline bool operator < (const CWVertex& v) const { return angle < v.angle; }
};

// ------------------------------------
//         ####     ####
//             #    #   #
//          ###     #   #
//             #    #   #
//         ####     ####
// ------------------------------------
void vtkYoungsMaterialInterfaceCellCut::cellInterface3D( 
   int ncoords,
   double coords[][3],
   int nedge,
   int cellEdges[][2],
   int ntetra,
   int tetraPointIds[][4],
   double fraction, double normal[3] , 
   bool useFractionAsDistance,
   int & np, int eids[], double weights[] ,
   int & nInside, int inPoints[],
   int & nOutside, int outPoints[] )
{
  // normalisation du vecteur normal si la norme >0
   double nlen2 = normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2];
   if( nlen2 > 0 )
   {
      double nlen = sqrt(nlen2);
      normal[0] /= nlen;
      normal[1] /= nlen;
      normal[2] /= nlen;
   }
   else
   {
      normal[0] = 1;
      normal[1] = 0;
      normal[2] = 0;
   }

   double dmin, dmax;
   dmin = dmax = coords[0][0]*normal[0] + coords[0][1]*normal[1] + coords[0][2]*normal[2];
   for(int i=0;i<ncoords;i++)
   {
      double d = coords[i][0]*normal[0] + coords[i][1]*normal[1] + coords[i][2]*normal[2];
      if( d<dmin ) dmin=d;
      else if( d>dmax ) dmax=d;
   }

  // compute plane's offset ( D parameter in Ax+By+Cz+D=0 )
   double d = useFractionAsDistance ? fraction : findTetraSetCuttingPlane( normal, fraction, ncoords, coords, ntetra, tetraPointIds );

  // compute vertex distances to interface plane
   double dist[MAX_CELL_POINTS];
   for(int i=0;i<ncoords;i++)
   {
      dist[i] = coords[i][0]*normal[0] + coords[i][1]*normal[1] + coords[i][2]*normal[2] + d;
   }

  // get in/out points
   nInside=0;
   nOutside=0;
   for(int i=0;i<ncoords;i++)
   {
      if( dist[i] <= 0.0 ) 
      {
   inPoints[nInside++] = i;
      }
      else
      {
   outPoints[nOutside++] = i;
      }
   }   

   double center[3] = {0,0,0};
   double polygon[MAX_CELL_POINTS][3];

  // compute intersections between edges and interface plane
   np = 0;
   for(int i=0;i<nedge;i++)
   {
      int e0 = cellEdges[i][0];
      int e1 = cellEdges[i][1];
      if( dist[e0]*dist[e1] < 0 )
      {
   double edist = dist[e1] - dist[e0];
   double t;
   if(edist!=0)
   {
      t = ( 0 - dist[e0] ) / edist ;
      Clamp(t,0,1);
   }
   else
   {
      t = 0;
   }
   
   for(int c=0;c<3;c++)
   {
      polygon[np][c] = coords[e0][c] + t * ( coords[e1][c] - coords[e0][c] ) ;
      center[c] += polygon[np][c];
   }
   eids[np*2+0] = e0;
   eids[np*2+1] = e1;
   weights[np] = t;
   np++;
      }
   }

  // tri des points
   if(np>3)
   {
     // calcul du centre du polygone
      for(int d=0;d<3;d++) center[d] /= np;

     // calcul de la direction dominante, pour retomber sur un cas 2D
      int maxDim = 0;
      if( fabs(normal[1]) > fabs(normal[maxDim]) ) maxDim=1;
      if( fabs(normal[2]) > fabs(normal[maxDim]) ) maxDim=2;
      int xd=0, yd=1;
      switch(maxDim)
      {
  case 0: xd=1; yd=2; break;
  case 1: xd=0; yd=2; break;
  case 2: xd=0; yd=1; break;
      }

     // calcul des angles des points du polygone
      CWVertex pts[MAX_CELL_POINTS];
      for(int i=0;i<np;i++)
      {
   double vec[3];
   for(int d=0;d<3;d++)
   {
      pts[i].coord[d] = polygon[i][d];
      vec[d] = polygon[i][d]-center[d];
   }
   
   pts[i].weight = weights[i];
   pts[i].eid[0] = eids[i*2+0];
   pts[i].eid[1] = eids[i*2+1];
   pts[i].angle = atan2( vec[yd], vec[xd] );
      }
      vtkstd::sort( pts , pts+np );
      for(int i=0;i<np;i++)
      {
   weights[i] = pts[i].weight;
   eids[i*2+0] = pts[i].eid[0];
   eids[i*2+1] = pts[i].eid[1];
      }
   }
}



double vtkYoungsMaterialInterfaceCellCut::findTetraSetCuttingPlane(
   const double normal[3],
   const double fraction,
   const int vertexCount,
   const double vertices[][3],
   const int tetraCount,
   const int tetras[][4]
   )
{
   vtkYoungsMaterialInterfaceCellCutInternals::Real3 N = { normal[0], normal[1], normal[2] };
   vtkYoungsMaterialInterfaceCellCutInternals::Real3 V[LOCAL_ARRAY_SIZE(vertexCount)];
   vtkYoungsMaterialInterfaceCellCutInternals::uchar4 tet[LOCAL_ARRAY_SIZE(tetraCount)];

   for(int i=0;i<vertexCount;i++)
   {
      V[i].x = vertices[i][0] - vertices[0][0] ;
      V[i].y = vertices[i][1] - vertices[0][1] ;
      V[i].z = vertices[i][2] - vertices[0][2] ;
   }

   vtkYoungsMaterialInterfaceCellCutInternals::Real3 vmin,vmax;
   vtkYoungsMaterialInterfaceCellCutInternals::Real scale;
   vmin = vmax = V[0];
   for(int i=1;i<vertexCount;i++)
   {
      if( V[i].x < vmin.x ) vmin.x = V[i].x;
      if( V[i].x > vmax.x ) vmax.x = V[i].x;
      if( V[i].y < vmin.y ) vmin.y = V[i].y;
      if( V[i].y > vmax.y ) vmax.y = V[i].y;
      if( V[i].z < vmin.z ) vmin.z = V[i].z;
      if( V[i].z > vmax.z ) vmax.z = V[i].z;
   }
   scale = vmax.x - vmin.x;
   if( (vmax.y-vmin.y) > scale ) scale = vmax.y-vmin.y;
   if( (vmax.z-vmin.z) > scale ) scale = vmax.z-vmin.z;
   for(int i=0;i<vertexCount;i++) V[i] /= scale;

   for(int i=0;i<tetraCount;i++)
   {
      tet[i].x = tetras[i][0];
      tet[i].y = tetras[i][1];
      tet[i].z = tetras[i][2];
      tet[i].w = tetras[i][3];
   }
   
   double dist0 = vertices[0][0]*normal[0] + vertices[0][1]*normal[1] + vertices[0][2]*normal[2];
   double d = dist0 + vtkYoungsMaterialInterfaceCellCutInternals::findTetraSetCuttingPlane(N, fraction, vertexCount, tetraCount, tet, V ) * scale;

   return - d;
}


// ------------------------------------
//         ####     ####
//             #    #   #
//          ###     #   #
//         #        #   #
//        #####     ####
// ------------------------------------

bool vtkYoungsMaterialInterfaceCellCut::cellInterface2D( 
   double points[][3],
   int nPoints,
   int triangles[][3], // TODO: int [] pour plus d'integration au niveau du dessus
   int nTriangles,
   double fraction, double normal[3] ,
   bool axisSymetric,
   bool useFractionAsDistance,
   int eids[4], double weights[2] ,
   int &polygonPoints, int polygonIds[],
   int &nRemPoints, int remPoints[]
   )
{
   double d = useFractionAsDistance ? fraction : findTriangleSetCuttingPlane( normal, fraction, nPoints, points, nTriangles, triangles , axisSymetric );

  // compute vertex distances to interface plane
   double dist[LOCAL_ARRAY_SIZE(nPoints)];
   for(int i=0;i<nPoints;i++)
   {
      dist[i] = points[i][0]*normal[0] + points[i][1]*normal[1] + points[i][2]*normal[2] + d;
   }

  // compute intersections between edges and interface line
   int np = 0;
   nRemPoints = 0;
   polygonPoints = 0;
   for(int i=0;i<nPoints;i++)
   {
      int edge[2];
      edge[0] = i;
      edge[1] = (i+1)%nPoints; 
      if( dist[i] <= 0.0 ) 
      {
   polygonIds[polygonPoints++] = i;
      }
      else
      {
   remPoints[nRemPoints++] = i;
      }
      if( np < 2 )
      {
   if( dist[edge[0]]*dist[edge[1]] < 0.0 )
   {
      double t = ( 0 - dist[edge[0]] ) / ( dist[edge[1]] - dist[edge[0]] );
      Clamp(t,0,1);
      eids[np*2+0] = edge[0];
      eids[np*2+1] = edge[1];
      weights[np] = t;
      np++;
      polygonIds[polygonPoints++] = -np;
      remPoints[nRemPoints++] = -np;
   }
      }
   }

   return (np==2);
}


double vtkYoungsMaterialInterfaceCellCut::findTriangleSetCuttingPlane(
   const double normal[3],
   const double fraction,
   const int vertexCount,
   const double vertices[][3],
   const int triangleCount,
   const int triangles[][3],
   bool axisSymetric
   )
{
   double d;

   vtkYoungsMaterialInterfaceCellCutInternals::uchar3 tri[LOCAL_ARRAY_SIZE(triangleCount)];
   for(int i=0;i<triangleCount;i++)
   {
      tri[i].x = triangles[i][0];
      tri[i].y = triangles[i][1];
      tri[i].z = triangles[i][2];
   }

   if( axisSymetric )
   {
      vtkYoungsMaterialInterfaceCellCutInternals::Real2 N = { normal[0], normal[1] };
      vtkYoungsMaterialInterfaceCellCutInternals::Real2 V[LOCAL_ARRAY_SIZE(vertexCount)];
      for(int i=0;i<vertexCount;i++)
      {
   V[i].x = vertices[i][0] - vertices[0][0] ;
   V[i].y = vertices[i][1] - vertices[0][1] ;
      }
      vtkYoungsMaterialInterfaceCellCutInternals::Real2 vmin,vmax;
      vtkYoungsMaterialInterfaceCellCutInternals::Real scale;
      vmin = vmax = V[0];
      for(int i=1;i<vertexCount;i++)
      {
   if( V[i].x < vmin.x ) vmin.x = V[i].x;
   if( V[i].x > vmax.x ) vmax.x = V[i].x;
   if( V[i].y < vmin.y ) vmin.y = V[i].y;
   if( V[i].y > vmax.y ) vmax.y = V[i].y;
      }
      scale = vmax.x - vmin.x;
      if( (vmax.y-vmin.y) > scale ) scale = vmax.y-vmin.y;
      for(int i=0;i<vertexCount;i++) V[i] /= scale;
      double dist0 = vertices[0][0]*normal[0] + vertices[0][1]*normal[1] ;
      d = dist0 + vtkYoungsMaterialInterfaceCellCutInternals::findTriangleSetCuttingCone(N, fraction, vertexCount, triangleCount, tri, V ) * scale;  
   }
   else
   {
      vtkYoungsMaterialInterfaceCellCutInternals::Real3 N = { normal[0], normal[1], normal[2] };
      vtkYoungsMaterialInterfaceCellCutInternals::Real3 V[LOCAL_ARRAY_SIZE(vertexCount)];
      for(int i=0;i<vertexCount;i++)
      {
   V[i].x = vertices[i][0] - vertices[0][0] ;
   V[i].y = vertices[i][1] - vertices[0][1] ;
   V[i].z = vertices[i][2] - vertices[0][2] ;
      }
      vtkYoungsMaterialInterfaceCellCutInternals::Real3 vmin,vmax;
      vtkYoungsMaterialInterfaceCellCutInternals::Real scale;
      vmin = vmax = V[0];
      for(int i=1;i<vertexCount;i++)
      {
   if( V[i].x < vmin.x ) vmin.x = V[i].x;
   if( V[i].x > vmax.x ) vmax.x = V[i].x;
   if( V[i].y < vmin.y ) vmin.y = V[i].y;
   if( V[i].y > vmax.y ) vmax.y = V[i].y;
   if( V[i].z < vmin.z ) vmin.z = V[i].z;
   if( V[i].z > vmax.z ) vmax.z = V[i].z;
      }
      scale = vmax.x - vmin.x;
      if( (vmax.y-vmin.y) > scale ) scale = vmax.y-vmin.y;
      if( (vmax.z-vmin.z) > scale ) scale = vmax.z-vmin.z;
      for(int i=0;i<vertexCount;i++) V[i] /= scale;
      double dist0 = vertices[0][0]*normal[0] + vertices[0][1]*normal[1] + vertices[0][2]*normal[2];
      d = dist0 + vtkYoungsMaterialInterfaceCellCutInternals::findTriangleSetCuttingPlane(N, fraction, vertexCount, triangleCount, tri, V ) * scale;
   }

   return - d;
}



