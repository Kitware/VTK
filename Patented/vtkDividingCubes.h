/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDividingCubes.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

     THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 4,719,585
     "Dividing Cubes System and Method for the Display of Surface Structures
     Contained Within the Interior Region of a Solid Body".
     Application of this software for commercial purposes requires 
     a license grant from GE. Contact:

         Carl B. Horton
         Sr. Counsel, Intellectual Property
         3000 N. Grandview Blvd., W-710
         Waukesha, WI  53188
         Phone:  (262) 513-4022
         E-Mail: Carl.Horton@med.ge.com

     for more information.

=========================================================================*/
// .NAME vtkDividingCubes - create points lying on isosurface
// .SECTION Description
// vtkDividingCubes is a filter that generates points lying on a surface
// of constant scalar value (i.e., an isosurface). Dense point clouds (i.e.,
// at screen resolution) will appear as a surface. Less dense clouds can be 
// used as a source to generate streamlines or to generate "transparent"
// surfaces. This filter is based on the generate program written by
// H. Cline, S. Ludke and W. Lorensen.
//
// The density of the point cloud is controlled by the Distance instance 
// variable. This is a distance value in global coordinates specifying the 
// approximate distance between points.

#ifndef __vtkDividingCubes_h
#define __vtkDividingCubes_h

#include "vtkStructuredPointsToPolyDataFilter.h"

class VTK_PATENTED_EXPORT vtkDividingCubes : public vtkStructuredPointsToPolyDataFilter
{
public:
  static vtkDividingCubes *New();
  vtkTypeRevisionMacro(vtkDividingCubes,vtkStructuredPointsToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set isosurface value.
  vtkSetMacro(Value,float);
  vtkGetMacro(Value,float);

  // Description:
  // Specify sub-voxel size at which to generate point.
  vtkSetClampMacro(Distance,float,1.0e-06,VTK_LARGE_FLOAT);
  vtkGetMacro(Distance,float);

  // Description:
  // Every "Increment" point is added to the list of points. This parameter, if
  // set to a large value, can be used to limit the number of points while
  // retaining good accuracy.
  vtkSetClampMacro(Increment,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(Increment,int);

protected:
  vtkDividingCubes();
  ~vtkDividingCubes();

  void Execute();
  void SubDivide(float origin[3], int dim[3], float h[3], float values[8]);

  float Value;
  float Distance;
  int Increment;

  // working variable
  int Count;

  vtkIdList *SubVoxelPts;
  vtkVoxel *SubVoxel;
  vtkFloatArray *SubVoxelScalars;
  vtkFloatArray *SubVoxelNormals;
private:
  vtkDividingCubes(const vtkDividingCubes&);  // Not implemented.
  void operator=(const vtkDividingCubes&);  // Not implemented.
};

#endif


