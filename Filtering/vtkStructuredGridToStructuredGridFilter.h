/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridToStructuredGridFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredGridToStructuredGridFilter - abstract filter class
// .SECTION Description
// vtkStructuredPointsToStructuredPointsFilter is an abstract filter class 
// whose subclasses take on input a structured grid  and generate a
// structured grid on output.

// .SECTION See Also
// vtkExtractGrid

#ifndef __vtkStructuredGridToStructuredGridFilter_h
#define __vtkStructuredGridToStructuredGridFilter_h

#include "vtkStructuredGridSource.h"

class VTK_FILTERING_EXPORT vtkStructuredGridToStructuredGridFilter : public vtkStructuredGridSource
{
public:
  vtkTypeMacro(vtkStructuredGridToStructuredGridFilter,vtkStructuredGridSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input Grid or filter.
  void SetInput(vtkStructuredGrid *input);
  vtkStructuredGrid *GetInput();

protected:
  vtkStructuredGridToStructuredGridFilter();
  ~vtkStructuredGridToStructuredGridFilter();

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkStructuredGridToStructuredGridFilter(const vtkStructuredGridToStructuredGridFilter&);  // Not implemented.
  void operator=(const vtkStructuredGridToStructuredGridFilter&);  // Not implemented.
};

#endif


