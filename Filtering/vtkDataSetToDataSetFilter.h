/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToDataSetFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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

class VTK_FILTERING_EXPORT vtkDataSetToDataSetFilter : public vtkDataSetSource
{

public:
  vtkTypeRevisionMacro(vtkDataSetToDataSetFilter,vtkDataSetSource);

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

  void ExecuteInformation();

private:
  vtkDataSetToDataSetFilter(const vtkDataSetToDataSetFilter&);  // Not implemented.
  void operator=(const vtkDataSetToDataSetFilter&);  // Not implemented.
};

#endif



