/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridOutlineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredGridOutlineFilter - create wireframe outline for structured grid
// .SECTION Description
// vtkStructuredGridOutlineFilter is a filter that generates a wireframe 
// outline of a structured grid (vtkStructuredGrid). Structured data is 
// topologically a cube, so the outline will have 12 "edges".

#ifndef __vtkStructuredGridOutlineFilter_h
#define __vtkStructuredGridOutlineFilter_h

#include "vtkStructuredGridToPolyDataAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkStructuredGridOutlineFilter : public vtkStructuredGridToPolyDataAlgorithm
{
public:
  static vtkStructuredGridOutlineFilter *New();
  vtkTypeRevisionMacro(vtkStructuredGridOutlineFilter,vtkStructuredGridToPolyDataAlgorithm);

protected:
  vtkStructuredGridOutlineFilter() {};
  ~vtkStructuredGridOutlineFilter() {};

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkStructuredGridOutlineFilter(const vtkStructuredGridOutlineFilter&);  // Not implemented.
  void operator=(const vtkStructuredGridOutlineFilter&);  // Not implemented.
};

#endif
