/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsToStructuredPointsFilter.h
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

class VTK_FILTERING_EXPORT vtkStructuredPointsToStructuredPointsFilter : public vtkStructuredPointsSource
{
public:
  vtkTypeRevisionMacro(vtkStructuredPointsToStructuredPointsFilter,vtkStructuredPointsSource);

  // Description:
  // Set / get the input data or filter.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();

protected:
  vtkStructuredPointsToStructuredPointsFilter() {this->NumberOfRequiredInputs = 1;};
  ~vtkStructuredPointsToStructuredPointsFilter() {};

  // Since input[0] and output are of same type, we can create this
  // method that defaults to just copying information.
  void ExecuteInformation();

  void ComputeInputUpdateExtents(vtkDataObject *output);

private:
  vtkStructuredPointsToStructuredPointsFilter(const vtkStructuredPointsToStructuredPointsFilter&);  // Not implemented.
  void operator=(const vtkStructuredPointsToStructuredPointsFilter&);  // Not implemented.
};

#endif


