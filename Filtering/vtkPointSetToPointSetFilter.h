/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetToPointSetFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

class VTK_FILTERING_EXPORT vtkPointSetToPointSetFilter : public vtkPointSetSource
{
public:
  vtkTypeMacro(vtkPointSetToPointSetFilter,vtkPointSetSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  
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
    {
      return this->vtkPointSetSource::GetOutput(idx);
    }

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

private:
  vtkPointSetToPointSetFilter(const vtkPointSetToPointSetFilter&);  // Not implemented.
  void operator=(const vtkPointSetToPointSetFilter&);  // Not implemented.
};

#endif


