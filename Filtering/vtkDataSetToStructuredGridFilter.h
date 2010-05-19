/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToStructuredGridFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetToStructuredGridFilter - abstract filter class
// .SECTION Description
// vtkDataSetToStructuredGridFilter is an abstract filter class whose 
// subclasses take as input any dataset and generate a structured
// grid on output.

#ifndef __vtkDataSetToStructuredGridFilter_h
#define __vtkDataSetToStructuredGridFilter_h

#include "vtkStructuredGridSource.h"

class vtkDataSet;

class VTK_FILTERING_EXPORT vtkDataSetToStructuredGridFilter : public vtkStructuredGridSource
{
public:
  vtkTypeMacro(vtkDataSetToStructuredGridFilter,vtkStructuredGridSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkDataSet *input);
  vtkDataSet *GetInput();

protected:
  vtkDataSetToStructuredGridFilter();
  ~vtkDataSetToStructuredGridFilter();

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkDataSetToStructuredGridFilter(const vtkDataSetToStructuredGridFilter&);  // Not implemented.
  void operator=(const vtkDataSetToStructuredGridFilter&);  // Not implemented.
};

#endif


