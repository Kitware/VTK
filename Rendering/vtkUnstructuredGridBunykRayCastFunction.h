/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridBunykRayCastFunction.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkUnstructuredGridBunykRayCastFunction - a superclass for ray casting functions

// .SECTION Description
// vtkUnstructuredGridBunykRayCastFunction is a concrete implementation of a
// ray cast function for unstructured grid data. This class was based on the
// paper "Simple, Fast, Robust Ray Casting of Irregular Grids" by Paul Bunyk,
// Arie Kaufmna, and Claudio Silva. This method is quite memory intensive 
// (with extra explicit copies of the data) and therefore should not be used
// for very large data. This method assumes that the input data is composed
// entirely of tetras - use vtkDataSetTriangleFilter before setting the input
// on the mapper.
//
// The basic idea of this method is as follows:
//
//   1) Enumerate the triangles. At each triangle have space for some 
//      information that will be used during rendering. This includes 
//      which tetra the triangles belong to, the plane equation and the  
//      Barycentric coefficients.
//
//   2) Keep a reference to all four triangles for each tetra.
//
//   3) At the beginning of each render, do the precomputation. This 
//      includes creating an array of transformed points (in view 
//      coordinates) and computing the view dependent info per triangle
//      (plane equations and barycentric coords in view space)
//
//   4) Find all front facing boundary triangles (a triangle is on the
//      boundary if it belongs to only one tetra). For each triangle,
//      find all pixels in the image that intersect the triangle, and
//      add this to the sorted (by depth) intersection list at each 
//      pixel.
//
//   5) For each ray cast, traverse the intersection list. At each
//      intersection, accumulate opacity and color contribution
//      per tetra along the ray until you reach an exiting triangle
//      (on the boundary). 
//

// .SECTION See Also
// vtkUnstructuredGridVolumeRayCastMapper

#ifndef __vtkUnstructuredGridBunykRayCastFunction_h
#define __vtkUnstructuredGridBunykRayCastFunction_h

#include "vtkUnstructuredGridVolumeRayCastFunction.h"

class vtkRenderer;
class vtkVolume;
class vtkUnstructuredGridVolumeRayCastMapper;
class vtkMatrix4x4;
class vtkPiecewiseFunction;
class vtkColorTransferFunction;
class vtkUnstructuredGrid;

// We manage the memory for the list of intersections ourself - this is the
// storage used. We keep 10,000 elements in each array, and we can have up to 
// 1,000 arrays.
#define VTK_BUNYKRCF_MAX_ARRAYS 1000
#define VTK_BUNYKRCF_ARRAY_SIZE 10000

class VTK_RENDERING_EXPORT vtkUnstructuredGridBunykRayCastFunction : public vtkUnstructuredGridVolumeRayCastFunction
{ 
public:
  static vtkUnstructuredGridBunykRayCastFunction *New();
  vtkTypeRevisionMacro(vtkUnstructuredGridBunykRayCastFunction,vtkUnstructuredGridVolumeRayCastFunction);
  virtual void PrintSelf(ostream& os, vtkIndent indent);
  
private:
  vtkUnstructuredGridBunykRayCastFunction(const vtkUnstructuredGridBunykRayCastFunction&);  // Not implemented.
  void operator=(const vtkUnstructuredGridBunykRayCastFunction&);  // Not implemented.
};

#endif







