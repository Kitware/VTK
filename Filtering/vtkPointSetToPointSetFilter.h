/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetToPointSetFilter.h
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
// .NAME vtkPointSetToPointSetFilter - abstract filter class 
// .SECTION Description
// vtkPointSetToPointSetFilter is an abstract filter class whose subclasses
// take as input a point set and generates a point set on output.  At a
// minimum, the concrete subclasses of vtkPointSetToPointSetFilter modify
// their point coordinates. They never modify their topological form,
// however.
//
// This is an abstract filter type. What that means is that the output of the
// filter is an abstract type (i.e., vtkPointSet), no matter what the input
// of the filter is. This can cause problems connecting together filters due
// to the change in dataset type. (For example, in a series of filters
// processing vtkPolyData, when a vtkPointSetToPointSetFilter or subclass is
// introduced into the pipeline, if the filter downstream of it takes
// vtkPolyData as input, the pipeline connection cannot be made.) To get
// around this problem, use one of the convenience methods to return a
// concrete type (e.g., vtkGetPolyDataOutput(), GetStructuredGridOutput(),
// etc.).

// .SECTION See Also
// vtkTransformFilter vtkWarpScalar vtkWarpTo vtkWarpVector

#ifndef __vtkPointSetToPointSetFilter_h
#define __vtkPointSetToPointSetFilter_h

#include "vtkPointSetSource.h"

class vtkPolyData;
class vtkStructuredGrid;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkPointSetToPointSetFilter : public vtkPointSetSource
{
public:
  vtkTypeMacro(vtkPointSetToPointSetFilter,vtkPointSetSource);
  
  // Description:
  // Specify the input data or filter.
  void SetInput(vtkPointSet *input);

  // Description:
  // Get the input data or filter.
  vtkPointSet *GetInput();

  // Description:
  // Get the output of this filter. If output is NULL, then input hasn't been
  // set, which is necessary for abstract filter objects.
  vtkPointSet *GetOutput();
  vtkPointSet *GetOutput(int idx)
    {return (vtkPointSet *) this->vtkPointSetSource::GetOutput(idx); };

  // Description:
  // Get the output as vtkPolyData. Performs run-time checking.
  vtkPolyData *GetPolyDataOutput();

  // Description:
  // Get the output as vtkStructuredGrid. Performs run-time checking.
  vtkStructuredGrid *GetStructuredGridOutput();

  // Description:
  // Get the output as vtkUnstructuredGrid. Performs run-time checking.
  vtkUnstructuredGrid *GetUnstructuredGridOutput();
  
  // Description:
  // By default copy the output update extent to the input
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

protected:
  vtkPointSetToPointSetFilter();
  ~vtkPointSetToPointSetFilter();
  vtkPointSetToPointSetFilter(const vtkPointSetToPointSetFilter&);
  void operator=(const vtkPointSetToPointSetFilter&);

};

#endif


