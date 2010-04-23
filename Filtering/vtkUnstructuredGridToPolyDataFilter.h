/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridToPolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnstructuredGridToPolyDataFilter - abstract filter class
// .SECTION Description
// vtkUnstructuredGridToPolyDataFilter is an abstract filter class whose
// subclasses take as input datasets of type vtkUnstructuredGrid and 
// generate polygonal data on output.

// .SECTION See Also
// vtkContourGrid

#ifndef __vtkUnstructuredGridToPolyDataFilter_h
#define __vtkUnstructuredGridToPolyDataFilter_h

#include "vtkPolyDataSource.h"

class vtkUnstructuredGrid;

class VTK_FILTERING_EXPORT vtkUnstructuredGridToPolyDataFilter : public vtkPolyDataSource
{
public:
  vtkTypeMacro(vtkUnstructuredGridToPolyDataFilter,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkUnstructuredGrid *input);
  vtkUnstructuredGrid *GetInput();
  
  // Description:
  // Do not let datasets return more than requested.
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

protected:
  vtkUnstructuredGridToPolyDataFilter();
  ~vtkUnstructuredGridToPolyDataFilter();

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkUnstructuredGridToPolyDataFilter(const vtkUnstructuredGridToPolyDataFilter&);  // Not implemented.
  void operator=(const vtkUnstructuredGridToPolyDataFilter&);  // Not implemented.
};

#endif


