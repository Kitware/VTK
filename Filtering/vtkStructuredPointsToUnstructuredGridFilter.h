/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsToUnstructuredGridFilter.h
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
// .NAME vtkStructuredPointsToUnstructuredGridFilter - abstract filter class
// .SECTION Description
// vtkStructuredPointsToUnstructuredGridFilter is an abstract filter class 
// whose subclasses take on input structured points and generate unstructured
// grid data on output.

// .SECTION See Also
// vtkClipVolume

#ifndef __vtkStructuredPointsToUnstructuredGridFilter_h
#define __vtkStructuredPointsToUnstructuredGridFilter_h

#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGridSource.h"

class VTK_FILTERING_EXPORT vtkStructuredPointsToUnstructuredGridFilter : public vtkUnstructuredGridSource
{
public:
  vtkTypeRevisionMacro(vtkStructuredPointsToUnstructuredGridFilter,vtkUnstructuredGridSource);

  // Description:
  // Set / get the input data or filter.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();

protected:
  vtkStructuredPointsToUnstructuredGridFilter() {this->NumberOfRequiredInputs = 1;};
  ~vtkStructuredPointsToUnstructuredGridFilter() {};

  void ComputeInputUpdateExtents(vtkDataObject *output);

private:
  vtkStructuredPointsToUnstructuredGridFilter(const vtkStructuredPointsToUnstructuredGridFilter&);  // Not implemented.
  void operator=(const vtkStructuredPointsToUnstructuredGridFilter&);  // Not implemented.
};

#endif


