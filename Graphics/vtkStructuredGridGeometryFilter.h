/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridGeometryFilter.h
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
// .NAME vtkStructuredGridGeometryFilter - extract geometry for structured grid
// .SECTION Description
// vtkStructuredGridGeometryFilter is a filter that extracts geometry from a
// structured grid. By specifying appropriate i-j-k indices, it is possible
// to extract a point, a curve, a surface, or a "volume". Depending upon the
// type of data, the curve and surface may be curved or planar. (The volume
// is actually a (n x m x o) region of points.)
//
// The extent specification is zero-offset. That is, the first k-plane in
// a 50x50x50 structured grid is given by (0,49, 0,49, 0,0).
//
// The output of this filter is affected by the structured grid blanking.
// If blanking is on, and a blanking array defined, then those cells 
// attached to blanked points are not output. (Blanking is a property of
// the input vtkStructuredGrid.)

// .SECTION Caveats
// If you don't know the dimensions of the input dataset, you can use a large
// number to specify extent (the number will be clamped appropriately). For 
// example, if the dataset dimensions are 50x50x50, and you want a the fifth 
// k-plane, you can use the extents (0,100, 0,100, 4,4). The 100 will 
// automatically be clamped to 49.

// .SECTION See Also
// vtkGeometryFilter vtkExtractGrid vtkStructuredGrid

#ifndef __vtkStructuredGridGeometryFilter_h
#define __vtkStructuredGridGeometryFilter_h

#include "vtkStructuredGridToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkStructuredGridGeometryFilter : public vtkStructuredGridToPolyDataFilter
{
public:
  static vtkStructuredGridGeometryFilter *New();
  vtkTypeMacro(vtkStructuredGridGeometryFilter,vtkStructuredGridToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the extent in topological coordinate range (imin,imax, jmin,jmax,
  // kmin,kmax).
  vtkGetVectorMacro(Extent,int,6);

  // Description:
  // Specify (imin,imax, jmin,jmax, kmin,kmax) indices.
  void SetExtent(int iMin, int iMax, int jMin, int jMax, int kMin, int kMax);

  // Description:
  // Specify (imin,imax, jmin,jmax, kmin,kmax) indices in array form.
  void SetExtent(int extent[6]);

protected:
  vtkStructuredGridGeometryFilter();
  ~vtkStructuredGridGeometryFilter() {}
  vtkStructuredGridGeometryFilter(const vtkStructuredGridGeometryFilter&);
  void operator=(const vtkStructuredGridGeometryFilter&);

  void Execute();
  void ComputeInputUpdateExtents( vtkDataObject *output );
  int Extent[6];
};

#endif


