/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsGeometryFilter.h
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
// .NAME vtkStructuredPointsGeometryFilter - extract geometry for structured points
// .SECTION Description
// vtkStructuredPointsGeometryFilter is a filter that extracts geometry from a
// structured points dataset. By specifying appropriate i-j-k indices (via the 
// "Extent" instance variable), it is possible to extract a point, a line, a 
// plane (i.e., image), or a "volume" from dataset. (Since the output is 
// of type polydata, the volume is actually a (n x m x o) region of points.)
//
// The extent specification is zero-offset. That is, the first k-plane in
// a 50x50x50 volume is given by (0,49, 0,49, 0,0).
// .SECTION Caveats
// If you don't know the dimensions of the input dataset, you can use a large
// number to specify extent (the number will be clamped appropriately). For 
// example, if the dataset dimensions are 50x50x50, and you want a the fifth 
// k-plane, you can use the extents (0,100, 0,100, 4,4). The 100 will 
// automatically be clamped to 49.

// .SECTION See Also
// vtkGeometryFilter vtkStructuredGridFilter

#ifndef __vtkStructuredPointsGeometryFilter_h
#define __vtkStructuredPointsGeometryFilter_h

#include "vtkStructuredPointsToPolyDataFilter.h"

class VTK_EXPORT vtkStructuredPointsGeometryFilter : public vtkStructuredPointsToPolyDataFilter
{
public:
  vtkTypeMacro(vtkStructuredPointsGeometryFilter,vtkStructuredPointsToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct with initial extent of all the data
  static vtkStructuredPointsGeometryFilter *New();

  
  // Description:
  // Set / get the extent (imin,imax, jmin,jmax, kmin,kmax) indices.
  void SetExtent(int extent[6]);
  void SetExtent(int iMin, int iMax, int jMin, int jMax, int kMin, int kMax);
  int *GetExtent() { return this->Extent;};

protected:
  vtkStructuredPointsGeometryFilter();
  ~vtkStructuredPointsGeometryFilter() {};
  vtkStructuredPointsGeometryFilter(const vtkStructuredPointsGeometryFilter&) {};
  void operator=(const vtkStructuredPointsGeometryFilter&) {};

  void Execute();
  int Extent[6];
};

#endif
