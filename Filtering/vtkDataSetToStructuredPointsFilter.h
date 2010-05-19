/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToStructuredPointsFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetToStructuredPointsFilter - abstract filter class
// .SECTION Description
// vtkDataSetToStructuredPointsFilter is an abstract filter class whose
// subclasses take as input any dataset and generate structured points 
// data on output.

// .SECTION See Also
// vtkGaussianSplatter vtkImplicitModeller vtkShepardMethod vtkVoxelModeller

#ifndef __vtkDataSetToStructuredPointsFilter_h
#define __vtkDataSetToStructuredPointsFilter_h

#include "vtkStructuredPointsSource.h"

class vtkDataSet;

class VTK_FILTERING_EXPORT vtkDataSetToStructuredPointsFilter : public vtkStructuredPointsSource
{
public:
  vtkTypeMacro(vtkDataSetToStructuredPointsFilter,vtkStructuredPointsSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkDataSet *input);
  vtkDataSet *GetInput();
  
protected:
  vtkDataSetToStructuredPointsFilter();
  ~vtkDataSetToStructuredPointsFilter();

  // All the DataSetToStructuredPointsFilters require all their input.
  void ComputeInputUpdateExtents(vtkDataObject *output);

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkDataSetToStructuredPointsFilter(const vtkDataSetToStructuredPointsFilter&);  // Not implemented.
  void operator=(const vtkDataSetToStructuredPointsFilter&);  // Not implemented.
};

#endif





