/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSubPixelPositionEdgels.h
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
// .NAME vtkSubPixelPositionEdgels - adjust edgel locations based on gradients.
// .SECTION Description
// vtkSubPixelPositionEdgels is a filter that takes a series of linked
// edgels (digital curves) and gradient maps as input. It then adjusts
// the edgel locations based on the gradient data. Specifically, the
// algorithm first determines the neighboring gradient magnitudes of
// an edgel using simple interpolation of its neighbors. It then fits
// the following three data points: negative gradient direction
// gradient magnitude, edgel gradient magnitude and positive gradient
// direction gradient magnitude to a quadratic function. It then
// solves this quadratic to find the maximum gradient location along
// the gradient orientation.  It then modifies the edgels location
// along the gradient orientation to the calculated maximum
// location. This algorithm does not adjust an edgel in the direction
// orthogonal to its gradient vector.

// .SECTION see also
// vtkImage vtkImageGradient vtkLinkEdgels

#ifndef __vtkSubPixelPositionEdgels_h
#define __vtkSubPixelPositionEdgels_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkStructuredPoints.h"

class VTK_GRAPHICS_EXPORT vtkSubPixelPositionEdgels : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkSubPixelPositionEdgels *New();
  vtkTypeMacro(vtkSubPixelPositionEdgels,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the gradient data for doing the position adjustments.
  void SetGradMaps(vtkStructuredPoints *gm);
  vtkStructuredPoints *GetGradMaps();

  // Description:
  // These methods can make the positioning look for a target scalar value
  // instead of looking for a maximum.
  vtkSetMacro(TargetFlag, int);
  vtkGetMacro(TargetFlag, int);
  vtkBooleanMacro(TargetFlag, int);
  vtkSetMacro(TargetValue, float);
  vtkGetMacro(TargetValue, float);
  
protected:
  vtkSubPixelPositionEdgels();
  ~vtkSubPixelPositionEdgels();

  // Usual data generation method
  void Execute();
  void Move(int xdim, int ydim, int zdim, int x, int y,
	    float *img, vtkDataArray *inVecs, 
	    float *result, int z, float *aspect, float *resultNormal);
  // extension for target instead of maximum
  int TargetFlag;
  float TargetValue;
private:
  vtkSubPixelPositionEdgels(const vtkSubPixelPositionEdgels&);  // Not implemented.
  void operator=(const vtkSubPixelPositionEdgels&);  // Not implemented.
};

#endif
