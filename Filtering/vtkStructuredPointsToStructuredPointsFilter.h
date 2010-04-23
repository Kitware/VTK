/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsToStructuredPointsFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredPointsToStructuredPointsFilter - abstract filter class
// .SECTION Description
// vtkStructuredPointsToStructuredPointsFilter is an abstract filter class 
// whose subclasses take on input structured points and generate
// structured points on output.

// .SECTION See Also
// vtkExtractVOI vtkImageDifference vtkSweptSurface
// vtkTransformStructuredPoints

#ifndef __vtkStructuredPointsToStructuredPointsFilter_h
#define __vtkStructuredPointsToStructuredPointsFilter_h

#include "vtkStructuredPointsSource.h"

class vtkImageData;

class VTK_FILTERING_EXPORT vtkStructuredPointsToStructuredPointsFilter : public vtkStructuredPointsSource
{
public:
  vtkTypeMacro(vtkStructuredPointsToStructuredPointsFilter,vtkStructuredPointsSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();

protected:
  vtkStructuredPointsToStructuredPointsFilter();
  ~vtkStructuredPointsToStructuredPointsFilter();

  // Since input[0] and output are of same type, we can create this
  // method that defaults to just copying information.
  void ExecuteInformation();

  void ComputeInputUpdateExtents(vtkDataObject *output);
  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkStructuredPointsToStructuredPointsFilter(const vtkStructuredPointsToStructuredPointsFilter&);  // Not implemented.
  void operator=(const vtkStructuredPointsToStructuredPointsFilter&);  // Not implemented.
};

#endif


