/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DS2DSF.h
  Language:  C++
  Date:      2/17/94
  Version:   1.8


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
// .NAME vtkDataSetToDataSetFilter - abstract filter class
// .SECTION Description
// vtkDataSetToDataSetFilter is an abstract filter class. Subclasses of
// vtkDataSetToDataSetFilter take a dataset as input and create a dataset as
// output. The form of the input geometry is not changed in these filters,
// only the point attributes (e.g. scalars, vectors, etc.).
//
// This is an abstract filter type. What that means is that the output of the
// filter is an abstract type (i.e., vtkDataSet), no matter what the input of
// the filter is. This can cause problems connecting together filters due to
// the change in dataset type. (For example, in a series of filters
// processing vtkPolyData, when a vtkDataSetToDataSetFilter or subclass is
// introduced into the pipeline, if the filter downstream of it takes
// vtkPolyData as input, the pipeline connection cannot be made.) To get
// around this problem, use one of the convenience methods to return a
// concrete type (e.g., vtkGetPolyDataOutput(), GetStructuredPointsOutput(),
// etc.).

// .SECTION See Also
// vtkBrownianPoints vtkProbeFilter vtkThresholdTextureCoords vtkDicer
// vtkElevationFilter vtkImplicitTextureCoords vtkTextureMapToBox 
// vtkTextureMapToPlane vtkVectorDot vtkVectorNorm

#ifndef __vtkDataSetToDataSetFilter_h
#define __vtkDataSetToDataSetFilter_h

#include "vtkDataSetSource.h"
#include "vtkDataSet.h"

class vtkPolyData;
class vtkStructuredPoints;
class vtkStructuredGrid;
class vtkUnstructuredGrid;
class vtkRectilinearGrid;

class VTK_EXPORT vtkDataSetToDataSetFilter : public vtkDataSetSource
{

public:
  vtkTypeMacro(vtkDataSetToDataSetFilter,vtkDataSetSource);

  // Description:
  // Specify the input data or filter.
  void SetInput(vtkDataSet *input);

  // Description:
  // Get the output of this filter. If output is NULL then input
  // hasn't been set which is necessary for abstract objects.
  vtkDataSet *GetOutput();
  vtkDataSet *GetOutput(int idx)
    {return (vtkDataSet *) this->vtkDataSetSource::GetOutput(idx); };

  // Description:
  // Get the output as vtkPolyData.
  vtkPolyData *GetPolyDataOutput();

  // Description:
  // Get the output as vtkStructuredPoints.
  vtkStructuredPoints *GetStructuredPointsOutput();

  // Description:
  // Get the output as vtkStructuredGrid.
  vtkStructuredGrid *GetStructuredGridOutput();

  // Description:
  // Get the output as vtkUnstructuredGrid.
  vtkUnstructuredGrid *GetUnstructuredGridOutput();

  // Description:
  // Get the output as vtkRectilinearGrid. 
  vtkRectilinearGrid *GetRectilinearGridOutput();
  
  // Description:
  // Get the input data or filter.
  vtkDataSet *GetInput();

  // Description:
  // By default copy the output update extent to the input
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

protected:
  vtkDataSetToDataSetFilter();
  ~vtkDataSetToDataSetFilter();
  vtkDataSetToDataSetFilter(const vtkDataSetToDataSetFilter&);
  void operator=(const vtkDataSetToDataSetFilter&);

  void ExecuteInformation();

};

#endif



