/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFiniteDifferenceGradientEstimator.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

class VTK_RENDERING_EXPORT vtkFiniteDifferenceGradientEstimator : public vtkEncodedGradientEstimator
{
public:
  vtkTypeMacro(vtkFiniteDifferenceGradientEstimator,vtkEncodedGradientEstimator);
  void PrintSelf( ostream& os, vtkIndent index );

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
