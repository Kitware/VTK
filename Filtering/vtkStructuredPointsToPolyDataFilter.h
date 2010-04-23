/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsToPolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

class vtkImageData;

class VTK_FILTERING_EXPORT vtkStructuredPointsToPolyDataFilter : public vtkPolyDataSource
{
public:
  vtkTypeMacro(vtkStructuredPointsToPolyDataFilter,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  
protected:  
  vtkStructuredPointsToPolyDataFilter();
  ~vtkStructuredPointsToPolyDataFilter();

  void ComputeInputUpdateExtents(vtkDataObject *output);
  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkStructuredPointsToPolyDataFilter(const vtkStructuredPointsToPolyDataFilter&);  // Not implemented.
  void operator=(const vtkStructuredPointsToPolyDataFilter&);  // Not implemented.
};

#endif


