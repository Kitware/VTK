/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinkEdgels.h
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
// .NAME vtkLinkEdgels - links edgels together to form digital curves.
// .SECTION Description
// vtkLinkEdgels links edgels into digital curves which are then stored 
// as polylines. The algorithm works one pixel at a time only looking at
// its immediate neighbors. There is a GradientThreshold that can be set 
// that eliminates any pixels with a smaller gradient value. This can
// be used as the lower threshold of a two value edgel thresholding. 
//
// For the remaining edgels, links are first tried for the four
// connected neighbors.  A successful neighbor will satisfy three
// tests. First both edgels must be above the gradient
// threshold. Second, the difference between the orientation between
// the two edgels (Alpha) and each edgels orientation (Phi) must be
// less than LinkThreshold. Third, the difference between the two
// edgels Phi values must be less than PhiThreshold.
// The most successful link is selected. The measure is simply the 
// sum of the three angle differences (actually stored as the sum of
// the cosines). If none of the four connect neighbors succeeds, then
// the eight connect neighbors are examined using the same method.
//  
// This filter requires gradient information so you will need to use
// a vtkImageGradient at some point prior to this filter.  Typically
// a vtkNonMaximumSuppression filter is also used. vtkThresholdEdgels
// can be used to complete the two value edgel thresholding as used
// in a Canny edge detector. The vtkSubpixelPositionEdgels filter 
// can also be used after this filter to adjust the edgel locations.

// .SECTION see also
// vtkImage vtkImageGradient vtkNonMaximumSuppression

#ifndef __vtkLinkEdgels_h
#define __vtkLinkEdgels_h

#include "vtkStructuredPointsToPolyDataFilter.h"

class VTK_EXPORT vtkLinkEdgels : public vtkStructuredPointsToPolyDataFilter
{
public:
  vtkTypeMacro(vtkLinkEdgels,vtkStructuredPointsToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct instance of vtkLinkEdgels with GradientThreshold set to 
  // 0.1, PhiThreshold set to 90 degrees and LinkThreshold set to 90 degrees.
  static vtkLinkEdgels *New();

  // Description:
  // Set/Get the threshold for Phi vs. Alpha link thresholding.
  vtkSetMacro(LinkThreshold,float);
  vtkGetMacro(LinkThreshold,float);

  // Description:
  // Set/get the threshold for Phi vs. Phi link thresholding.
  vtkSetMacro(PhiThreshold,float);
  vtkGetMacro(PhiThreshold,float);

  // Description:
  // Set/Get the threshold for image gradient thresholding.
  vtkSetMacro(GradientThreshold,float);
  vtkGetMacro(GradientThreshold,float);

protected:
  vtkLinkEdgels();
  ~vtkLinkEdgels() {};
  vtkLinkEdgels(const vtkLinkEdgels&);
  void operator=(const vtkLinkEdgels&);

  void Execute();
  void LinkEdgels(int xdim, int ydim,float *image, vtkDataArray *inVectors,
		  vtkCellArray *newLines, vtkPoints *newPts,
		  vtkFloatArray *outScalars, vtkFloatArray *outVectors,
		  int z);
  float GradientThreshold;
  float PhiThreshold;
  float LinkThreshold;
};

#endif
