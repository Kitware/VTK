/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridOutlineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRectilinearGridOutlineFilter - create wireframe outline for a rectilinear grid.
// .SECTION Description
// vtkRectilinearGridOutlineFilter works in parallel.  There is no reason.
// to use this filter if you are not breaking the processing into pieces.
// With one piece you can simply use vtkOutlineFilter.  This filter
// ignores internal edges when the extent is not the whole extent.

#ifndef __vtkRectilinearGridOutlineFilter_h
#define __vtkRectilinearGridOutlineFilter_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSPARALLEL_EXPORT vtkRectilinearGridOutlineFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkRectilinearGridOutlineFilter *New();
  vtkTypeMacro(vtkRectilinearGridOutlineFilter,vtkPolyDataAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkRectilinearGridOutlineFilter() {}
  ~vtkRectilinearGridOutlineFilter() {}
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkRectilinearGridOutlineFilter(const vtkRectilinearGridOutlineFilter&);  // Not implemented.
  void operator=(const vtkRectilinearGridOutlineFilter&);  // Not implemented.
};

#endif
