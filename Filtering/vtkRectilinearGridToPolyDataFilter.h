/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridToPolyDataFilter.h
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
// .NAME vtkRectilinearGridToPolyDataFilter - abstract filter class
// .SECTION Description
// vtkRectilinearGridToPolyDataFilter is a filter whose subclasses take as
// input rectilinear grid datasets and generate polygonal data on output.

// .SECTION See Also
// vtkRectilinearGridGeometryFilter vtkRectilinearGridOutlineFilter

#ifndef __vtkRectilinearGridToPolyDataFilter_h
#define __vtkRectilinearGridToPolyDataFilter_h

#include "vtkPolyDataSource.h"
#include "vtkRectilinearGrid.h"

class VTK_FILTERING_EXPORT vtkRectilinearGridToPolyDataFilter : public vtkPolyDataSource
{
public:
  vtkTypeRevisionMacro(vtkRectilinearGridToPolyDataFilter,vtkPolyDataSource);

  // Description:
  // Set / get the input Grid or filter.
  void SetInput(vtkRectilinearGrid *input);
  vtkRectilinearGrid *GetInput();

protected:
  vtkRectilinearGridToPolyDataFilter() {this->NumberOfRequiredInputs = 1;};
  ~vtkRectilinearGridToPolyDataFilter() {};
private:
  vtkRectilinearGridToPolyDataFilter(const vtkRectilinearGridToPolyDataFilter&);  // Not implemented.
  void operator=(const vtkRectilinearGridToPolyDataFilter&);  // Not implemented.
};

#endif


