/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkYoungsMaterialInterfaceCellCut.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkYoungsMaterialInterfaceCellCut - computes material interface plane/line
//
// .SECTION Description
// vtkYoungsMaterialInterfaceCellCut Contains methods to compute the interface polygon
// (for 3D cells) or line (for 2D cells) given a normal, a volume fraction and a set of simplices.
//
// .SECTION Thanks
// This file is part of the generalized Youngs material interface reconstruction algorithm contributed by
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
// BP12, F-91297 Arpajon, France. <br>
// Implementation by Thierry Carrard (CEA)

#ifndef __vtkYoungsMaterialInterfaceCellCut_h
#define __vtkYoungsMaterialInterfaceCellCut_h

#include "vtkObject.h"

class VTK_GRAPHICS_EXPORT vtkYoungsMaterialInterfaceCellCut
{
   public:

      enum {
   MAX_CELL_POINTS = 128,
   MAX_CELL_TETRAS = 128
      };


     // ------------------------------------
     //         ####     ####
     //             #    #   #
     //          ###     #   #
     //             #    #   #
     //         ####     ####
     // ------------------------------------

      static void cellInterface3D( 
   
  // Inputs
   int ncoords,
   double coords[][3],
   int nedge,
   int cellEdges[][2],
   int ntetra,
   int tetraPointIds[][4],
   double fraction, double normal[3] , 
   bool useFractionAsDistance,

  // Outputs
   int & np, int eids[], double weights[] ,
   int & nInside, int inPoints[],
   int & nOutside, int outPoints[] );


  static double findTetraSetCuttingPlane(
     const double normal[3],
     const double fraction,
     const int vertexCount,
     const double vertices[][3],
     const int tetraCount,
     const int tetras[][4] );


     // ------------------------------------
     //         ####     ####
     //             #    #   #
     //          ###     #   #
     //         #        #   #
     //        #####     ####
     // ------------------------------------

      static bool cellInterface2D( 

  // Inputs
   double points[][3],
   int nPoints,
   int triangles[][3], 
   int nTriangles,
   double fraction, double normal[3] ,
   bool axisSymetric,
   bool useFractionAsDistance,

  // Outputs
   int eids[4], double weights[2] ,
   int &polygonPoints, int polygonIds[],
   int &nRemPoints, int remPoints[] );


  static double findTriangleSetCuttingPlane(
     const double normal[3],
     const double fraction,
     const int vertexCount,
     const double vertices[][3],
     const int triangleCount,
     const int triangles[][3],
     bool axisSymetric=false );


} ;

#endif /* __vtkYoungsMaterialInterfaceCellCut_h */

