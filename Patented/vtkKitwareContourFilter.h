/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKitwareContourFilter.h
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
// .NAME vtkKitwareContourFilter - generate isosurfaces/isolines from scalar values
// .SECTION Description
// vtkKitwareContourFilter is a filter that takes as input any dataset and 
// generates on output isosurfaces and/or isolines. The exact form 
// of the output depends upon the dimensionality of the input data. 
// Data consisting of 3D cells will generate isosurfaces, data 
// consisting of 2D cells will generate isolines, and data with 1D 
// or 0D cells will generate isopoints. Combinations of output type 
// are possible if the input dimension is mixed.
//
// This filter will identify special dataset types (e.g., structured
// points) and use the appropriate specialized filter to process the
// data. For examples, if the input dataset type is a volume, this
// filter will create an internal vtkSyncronizedTemplates3D instance 
// and use it. This gives much better performance on StructuredPoints 
// and StructuredGrids.
// 
// To use this filter you must specify one or more contour values.
// You can either use the method SetValue() to specify each contour
// value, or use GenerateValues() to generate a series of evenly
// spaced contours. It is also possible to accelerate the operation of
// this filter (at the cost of extra memory) by using a
// vtkScalarTree. A scalar tree is used to quickly locate cells that
// contain a contour surface. This is especially effective if multiple
// contours are being extracted. If you want to use a scalar tree,
// invoke the method UseScalarTreeOn().

// .SECTION Caveats
// For StructuredPoints And StructuredGrids, normals are computed
// by default, but it is an expensive computation.  Processing for 
// other data set types has not been extended to include normal 
// computation.  In the mean time, use vtkPolyDataNormals to compute 
// the surface normals.

// .SECTION See Also
// vtkSynchronizedTemplates3D vtkSynchronizedTemplates2D 
// vtkGridSynchronizedTemplates3D

#ifndef __vtkKitwareContourFilter_h
#define __vtkKitwareContourFilter_h

#include "vtkContourFilter.h"

class VTK_PATENTED_EXPORT vtkKitwareContourFilter : public vtkContourFilter
{
public:
  vtkTypeMacro(vtkKitwareContourFilter,vtkContourFilter);

  // Description:
  // Construct object with initial range (0,1) and single contour value
  // of 0.0.
  static vtkKitwareContourFilter *New();

protected:
  vtkKitwareContourFilter();
  ~vtkKitwareContourFilter();
  vtkKitwareContourFilter(const vtkKitwareContourFilter&);
  void operator=(const vtkKitwareContourFilter&);

  void ComputeInputUpdateExtents(vtkDataObject *data);
  void Execute();
  void ExecuteInformation();

  //special contouring for structured points
  void StructuredPointsContour(int dim); 
  //special contouring for structured grid
  void StructuredGridContour(int dim);
  //default if not structured data
  void DataSetContour();
};

#endif


