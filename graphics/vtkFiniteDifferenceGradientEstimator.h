/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFiniteDifferenceGradientEstimator.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

class VTK_EXPORT vtkFiniteDifferenceGradientEstimator : public vtkEncodedGradientEstimator
{
public:
  vtkFiniteDifferenceGradientEstimator();
  ~vtkFiniteDifferenceGradientEstimator();
  const char *GetClassName() {return "vtkFiniteDifferenceGradientEstimator";};
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Construct a vtkFiniteDifferenceGradientEstimator with
  // a SampleSpacingInVoxels of 1.
  static vtkFiniteDifferenceGradientEstimator *New() {
    return new vtkFiniteDifferenceGradientEstimator;};

  // Description:
  // Set/Get the spacing between samples for the finite differences
  // method used to compute the normal. This spacing is in voxel units.
  vtkSetMacro( SampleSpacingInVoxels, int );
  vtkGetMacro( SampleSpacingInVoxels, int );

  // The sample spacing between samples taken for the normal estimation
  int SampleSpacingInVoxels;

protected:

  // Description:
  // Recompute the encoded normals and gradient magnitudes.
  void UpdateNormals( void );
}; 


#endif
