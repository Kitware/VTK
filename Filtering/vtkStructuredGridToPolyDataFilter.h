/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridToPolyDataFilter.h
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
// .NAME vtkStructuredGridToPolyDataFilter - abstract filter class
// .SECTION Description
// vtkStructuredGridToPolyDataFilter is a filter whose subclasses take as input
// structured grid datasets and generate polygonal data on output. 

// .SECTION See Also
// vtkStructuredGridGeometryFilter vtkStructuredGridOutlineFilter

#ifndef __vtkStructuredGridToPolyDataFilter_h
#define __vtkStructuredGridToPolyDataFilter_h

#include "vtkPolyDataSource.h"
#include "vtkStructuredGrid.h"

class VTK_FILTERING_EXPORT vtkStructuredGridToPolyDataFilter : public vtkPolyDataSource
{
public:
  vtkTypeRevisionMacro(vtkStructuredGridToPolyDataFilter,vtkPolyDataSource);

  // Description:
  // Set / get the input Grid or filter.
  void SetInput(vtkStructuredGrid *input);
  vtkStructuredGrid *GetInput();
  
protected:  
  vtkStructuredGridToPolyDataFilter() {this->NumberOfRequiredInputs = 1;};
  ~vtkStructuredGridToPolyDataFilter() {};

private:
  vtkStructuredGridToPolyDataFilter(const vtkStructuredGridToPolyDataFilter&);  // Not implemented.
  void operator=(const vtkStructuredGridToPolyDataFilter&);  // Not implemented.
};

#endif


