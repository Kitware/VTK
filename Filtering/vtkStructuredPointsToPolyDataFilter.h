/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsToPolyDataFilter.h
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
// .NAME vtkStructuredPointsToPolyDataFilter - abstract filter class
// .SECTION Description
// vtkStructuredPointsToPolyDataFilter is an abstract filter class whose
// subclasses take on input structured points and generate polygonal 
// data on output.

// .SECTION See Also
// vtkDividingCubes vtkMarchingCubes vtkMarchingSquares
// vtkRecursiveDividingCubes vtkImageDataGeometryFilter

#ifndef __vtkStructuredPointsToPolyDataFilter_h
#define __vtkStructuredPointsToPolyDataFilter_h

#include "vtkPolyDataSource.h"
#include "vtkImageData.h"
#include "vtkStructuredPoints.h"

class VTK_FILTERING_EXPORT vtkStructuredPointsToPolyDataFilter : public vtkPolyDataSource
{
public:
  vtkTypeRevisionMacro(vtkStructuredPointsToPolyDataFilter,vtkPolyDataSource);

  // Description:
  // Set / get the input data or filter.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  
protected:  
  vtkStructuredPointsToPolyDataFilter() {this->NumberOfRequiredInputs = 1;};
  ~vtkStructuredPointsToPolyDataFilter() {};

  void ComputeInputUpdateExtents(vtkDataObject *output);
private:
  vtkStructuredPointsToPolyDataFilter(const vtkStructuredPointsToPolyDataFilter&);  // Not implemented.
  void operator=(const vtkStructuredPointsToPolyDataFilter&);  // Not implemented.
};

#endif


