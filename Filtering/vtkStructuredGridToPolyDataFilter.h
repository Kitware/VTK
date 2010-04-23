/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridToPolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

class vtkStructuredGrid;

class VTK_FILTERING_EXPORT vtkStructuredGridToPolyDataFilter : public vtkPolyDataSource
{
public:
  vtkTypeMacro(vtkStructuredGridToPolyDataFilter,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input Grid or filter.
  void SetInput(vtkStructuredGrid *input);
  vtkStructuredGrid *GetInput();
  
protected:  
  vtkStructuredGridToPolyDataFilter();
  ~vtkStructuredGridToPolyDataFilter();

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkStructuredGridToPolyDataFilter(const vtkStructuredGridToPolyDataFilter&);  // Not implemented.
  void operator=(const vtkStructuredGridToPolyDataFilter&);  // Not implemented.
};

#endif


