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

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkStructuredGridOutlineFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkStructuredGridOutlineFilter *New();
  vtkTypeMacro(vtkStructuredGridOutlineFilter,vtkPolyDataAlgorithm);

protected:
  vtkStructuredGridOutlineFilter() {};
  ~vtkStructuredGridOutlineFilter() {};

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkStructuredGridOutlineFilter(const vtkStructuredGridOutlineFilter&);  // Not implemented.
  void operator=(const vtkStructuredGridOutlineFilter&);  // Not implemented.
};

#endif
// VTK-HeaderTest-Exclude: vtkStructuredGridOutlineFilter.h
