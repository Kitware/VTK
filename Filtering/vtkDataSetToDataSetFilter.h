/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToDataSetFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
// vtkElevationFilter vtkImplicitTextureCoords  
// vtkTextureMapToPlane vtkVectorDot vtkVectorNorm

#ifndef __vtkDataSetToDataSetFilter_h
#define __vtkDataSetToDataSetFilter_h

#include "vtkDataSetSource.h"

class vtkDataSet;
class vtkPolyData;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkStructuredPoints;
class vtkUnstructuredGrid;

class VTK_FILTERING_EXPORT vtkDataSetToDataSetFilter : public vtkDataSetSource
{

public:
  vtkTypeMacro(vtkDataSetToDataSetFilter,vtkDataSetSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the input data or filter.
  void SetInput(vtkDataSet *input);

  // Description:
  // Get the output of this filter. If output is NULL then input
  // hasn't been set which is necessary for abstract objects.
  vtkDataSet *GetOutput();
  vtkDataSet *GetOutput(int idx);

  // Description:
  // Get the output as vtkPolyData.
  virtual vtkPolyData *GetPolyDataOutput();

  // Description:
  // Get the output as vtkStructuredPoints.
  virtual vtkStructuredPoints *GetStructuredPointsOutput();

  // Description:
  // Get the output as vtkStructuredGrid.
  virtual vtkStructuredGrid *GetStructuredGridOutput();

  // Description:
  // Get the output as vtkUnstructuredGrid.
  virtual vtkUnstructuredGrid *GetUnstructuredGridOutput();

  // Description:
  // Get the output as vtkRectilinearGrid. 
  virtual vtkRectilinearGrid *GetRectilinearGridOutput();
  
  // Description:
  // Get the input data or filter.
  vtkDataSet *GetInput();

  // Description:
  // By default copy the output update extent to the input
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

  // Description:
  // Transform pipeline requests from executives into old-style
  // pipeline calls.  This works with the
  // vtkStreamingDemandDrivenPipeline executive to maintain backward
  // compatibility for filters written as subclasses of vtkSource.
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

protected:
  vtkDataSetToDataSetFilter();
  ~vtkDataSetToDataSetFilter();

  void ExecuteInformation();

  virtual int FillInputPortInformation(int, vtkInformation*);

  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestDataObject(vtkInformation* request, 
                           vtkInformationVector** inputVector, 
                           vtkInformationVector* outputVector);

private:
  vtkDataSetToDataSetFilter(const vtkDataSetToDataSetFilter&);  // Not implemented.
  void operator=(const vtkDataSetToDataSetFilter&);  // Not implemented.
};

#endif



