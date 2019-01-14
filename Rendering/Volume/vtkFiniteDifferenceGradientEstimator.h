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

/**
 * @class   vtkFiniteDifferenceGradientEstimator
 * @brief   Use finite differences to estimate gradient.
 *
 *
 *  vtkFiniteDifferenceGradientEstimator is a concrete subclass of
 *  vtkEncodedGradientEstimator that uses a central differences technique to
 *  estimate the gradient. The gradient at some sample location (x,y,z)
 *  would be estimated by:
 *
 *       nx = (f(x-dx,y,z) - f(x+dx,y,z)) / 2*dx;
 *       ny = (f(x,y-dy,z) - f(x,y+dy,z)) / 2*dy;
 *       nz = (f(x,y,z-dz) - f(x,y,z+dz)) / 2*dz;
 *
 *  This value is normalized to determine a unit direction vector and a
 *  magnitude. The normal is computed in voxel space, and
 *  dx = dy = dz = SampleSpacingInVoxels. A scaling factor is applied to
 *  convert this normal from voxel space to world coordinates.
 *
 * @sa
 * vtkEncodedGradientEstimator
*/

#ifndef vtkFiniteDifferenceGradientEstimator_h
#define vtkFiniteDifferenceGradientEstimator_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkEncodedGradientEstimator.h"

class VTKRENDERINGVOLUME_EXPORT vtkFiniteDifferenceGradientEstimator : public vtkEncodedGradientEstimator
{
public:
  vtkTypeMacro(vtkFiniteDifferenceGradientEstimator,vtkEncodedGradientEstimator);
  void PrintSelf( ostream& os, vtkIndent indent ) override;

  /**
   * Construct a vtkFiniteDifferenceGradientEstimator with
   * a SampleSpacingInVoxels of 1.
   */
  static vtkFiniteDifferenceGradientEstimator *New();

  //@{
  /**
   * Set/Get the spacing between samples for the finite differences
   * method used to compute the normal. This spacing is in voxel units.
   */
  vtkSetMacro( SampleSpacingInVoxels, int );
  vtkGetMacro( SampleSpacingInVoxels, int );
  //@}

  // The sample spacing between samples taken for the normal estimation
  int SampleSpacingInVoxels;

protected:
  vtkFiniteDifferenceGradientEstimator();
  ~vtkFiniteDifferenceGradientEstimator() override;

  /**
   * Recompute the encoded normals and gradient magnitudes.
   */
  void UpdateNormals( void ) override;

private:
  vtkFiniteDifferenceGradientEstimator(const vtkFiniteDifferenceGradientEstimator&) = delete;
  void operator=(const vtkFiniteDifferenceGradientEstimator&) = delete;
};


#endif
