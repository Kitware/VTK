/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFiniteDifferenceGradientEstimator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkFiniteDifferenceGradientEstimator - Use finite differences to estimate gradient.
//
// .SECTION Description
//  vtkFiniteDifferenceGradientEstimator is a concrete subclass of 
//  vtkEncodedGradientEstimator that uses a central differences technique to
//  estimate the gradient. The gradient at some sample location (x,y,z)
//  would be estimated by:
//      
//       nx = (f(x-dx,y,z) - f(x+dx,y,z)) / 2*dx;
//       ny = (f(x,y-dy,z) - f(x,y+dy,z)) / 2*dy;
//       nz = (f(x,y,z-dz) - f(x,y,z+dz)) / 2*dz;
//
//  This value is normalized to determine a unit direction vector and a
//  magnitude. The normal is computed in voxel space, and 
//  dx = dy = dz = SampleSpacingInVoxels. A scaling factor is applied to
//  convert this normal from voxel space to world coordinates.
//
// .SECTION see also
// vtkEncodedGradientEstimator

#ifndef __vtkFiniteDifferenceGradientEstimator_h
#define __vtkFiniteDifferenceGradientEstimator_h

#include "vtkEncodedGradientEstimator.h"

class VTK_VOLUMERENDERING_EXPORT vtkFiniteDifferenceGradientEstimator : public vtkEncodedGradientEstimator
{
public:
  vtkTypeMacro(vtkFiniteDifferenceGradientEstimator,vtkEncodedGradientEstimator);
  void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Construct a vtkFiniteDifferenceGradientEstimator with
  // a SampleSpacingInVoxels of 1.
  static vtkFiniteDifferenceGradientEstimator *New();

  // Description:
  // Set/Get the spacing between samples for the finite differences
  // method used to compute the normal. This spacing is in voxel units.
  vtkSetMacro( SampleSpacingInVoxels, int );
  vtkGetMacro( SampleSpacingInVoxels, int );

  // The sample spacing between samples taken for the normal estimation
  int SampleSpacingInVoxels;

protected:
  vtkFiniteDifferenceGradientEstimator();
  ~vtkFiniteDifferenceGradientEstimator();


  // Description:
  // Recompute the encoded normals and gradient magnitudes.
  void UpdateNormals( void );
private:
  vtkFiniteDifferenceGradientEstimator(const vtkFiniteDifferenceGradientEstimator&);  // Not implemented.
  void operator=(const vtkFiniteDifferenceGradientEstimator&);  // Not implemented.
}; 


#endif
