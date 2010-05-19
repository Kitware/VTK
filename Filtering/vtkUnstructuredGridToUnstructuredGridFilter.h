/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridToUnstructuredGridFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnstructuredGridToUnstructuredGridFilter - abstract filter class
// .SECTION Description

// .SECTION See Also
// vtkExtractGrid

#ifndef __vtkUnstructuredGridToUnstructuredGridFilter_h
#define __vtkUnstructuredGridToUnstructuredGridFilter_h

#include "vtkUnstructuredGridSource.h"

class VTK_FILTERING_EXPORT vtkUnstructuredGridToUnstructuredGridFilter : public vtkUnstructuredGridSource
{
public:
  vtkTypeMacro(vtkUnstructuredGridToUnstructuredGridFilter,vtkUnstructuredGridSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input Grid or filter.
  void SetInput(vtkUnstructuredGrid *input);
  vtkUnstructuredGrid *GetInput();

  virtual int FillInputPortInformation(int, vtkInformation*);

protected:
  vtkUnstructuredGridToUnstructuredGridFilter();
  ~vtkUnstructuredGridToUnstructuredGridFilter();
private:
  vtkUnstructuredGridToUnstructuredGridFilter(const vtkUnstructuredGridToUnstructuredGridFilter&);  // Not implemented.
  void operator=(const vtkUnstructuredGridToUnstructuredGridFilter&);  // Not implemented.
};

#endif


