/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridToPolyDataFilter.h
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
#include "vtkUnstructuredGrid.h"

class VTK_FILTERING_EXPORT vtkUnstructuredGridToPolyDataFilter : public vtkPolyDataSource
{
public:
  vtkTypeRevisionMacro(vtkUnstructuredGridToPolyDataFilter,vtkPolyDataSource);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkUnstructuredGrid *input);
  vtkUnstructuredGrid *GetInput();
  
  // Description:
  // Do not let datasets return more than requested.
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

protected:
  vtkUnstructuredGridToPolyDataFilter() {this->NumberOfRequiredInputs = 1;}
  ~vtkUnstructuredGridToPolyDataFilter() {}
  
  
private:
  vtkUnstructuredGridToPolyDataFilter(const vtkUnstructuredGridToPolyDataFilter&);  // Not implemented.
  void operator=(const vtkUnstructuredGridToPolyDataFilter&);  // Not implemented.
};

#endif


