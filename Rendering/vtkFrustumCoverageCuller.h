
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrustumCoverageCuller.h
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
// .NAME vtkFrustumCoverageCuller - cull props based on frustum coverage
// .SECTION Description
// vtkFrustumCoverageCuller will cull props based on the coverage in
// the view frustum. The coverage is computed by enclosing the prop in
// a bounding sphere, projecting that to the viewing coordinate system, then
// taking a slice through the view frustum at the center of the sphere. This
// results in a circle on the plane slice through the view frustum. This
// circle is enclosed in a squared, and the fraction of the plane slice that
// this square covers is the coverage. This is a number between 0 and 1.
// If the number is less than the MinumumCoverage, the allocated render time
// for that prop is set to zero. If it is greater than the MaximumCoverage,
// the allocated render time is set to 1.0. In between, a linear ramp is used
// to convert coverage into allocated render time.


// .SECTION see also
// vtkCuller

#ifndef __vtkFrustumCoverageCuller_h
#define __vtkFrustumCoverageCuller_h

#include "vtkCuller.h"

#define VTK_CULLER_SORT_NONE          0
#define VTK_CULLER_SORT_FRONT_TO_BACK 1
#define VTK_CULLER_SORT_BACK_TO_FRONT 2

class vtkProp;
class vtkRenderer;

class VTK_RENDERING_EXPORT vtkFrustumCoverageCuller : public vtkCuller
{
public:
  static vtkFrustumCoverageCuller *New();
  vtkTypeMacro(vtkFrustumCoverageCuller,vtkCuller);
  void PrintSelf(ostream& os,vtkIndent indent);

  // Description:
  // Set/Get the minimum coverage - props with less coverage than this
  // are given no time to render (they are culled)
  vtkSetMacro( MinimumCoverage, float );
  vtkGetMacro( MinimumCoverage, float );

  // Description:
  // Set/Get the maximum coverage - props with more coverage than this are
  // given an allocated render time of 1.0 (the maximum)
  vtkSetMacro( MaximumCoverage, float );
  vtkGetMacro( MaximumCoverage, float );

  // Description:
  // Set the sorting style - none, front-to-back or back-to-front
  // The default is none
  vtkSetClampMacro( SortingStyle, int,
	VTK_CULLER_SORT_NONE, VTK_CULLER_SORT_BACK_TO_FRONT );
  vtkGetMacro(SortingStyle,int);
  void SetSortingStyleToNone()
	{this->SetSortingStyle(VTK_CULLER_SORT_NONE);};
  void SetSortingStyleToBackToFront()
    {this->SetSortingStyle(VTK_CULLER_SORT_BACK_TO_FRONT);};
  void SetSortingStyleToFrontToBack()
    {this->SetSortingStyle(VTK_CULLER_SORT_FRONT_TO_BACK);};
  const char *GetSortingStyleAsString(void);

//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
  // Perform the cull operation
  // This method should only be called by vtkRenderer as part of
  // the render process
  float Cull( vtkRenderer *ren, vtkProp **propList,
	      int& listLength, int& initialized );
//ETX

protected:
  vtkFrustumCoverageCuller();
  ~vtkFrustumCoverageCuller() {};

  float        MinimumCoverage;
  float        MaximumCoverage;
  int          SortingStyle;
private:
  vtkFrustumCoverageCuller(const vtkFrustumCoverageCuller&);  // Not implemented.
  void operator=(const vtkFrustumCoverageCuller&);  // Not implemented.
};

                                         
#endif
