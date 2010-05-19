/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToUnstructuredGridFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetToUnstructuredGridFilter - abstract filter class
// .SECTION Description
// vtkDataSetToUnstructuredGridFilter is an abstract filter class whose 
// subclasses take as input any dataset and generate an unstructured
// grid on output.

// .SECTION See Also
// vtkAppendFilter vtkConnectivityFilter vtkExtractGeometry
// vtkShrinkFilter vtkThreshold

#ifndef __vtkDataSetToUnstructuredGridFilter_h
#define __vtkDataSetToUnstructuredGridFilter_h

#include "vtkUnstructuredGridSource.h"

class vtkDataSet;

class VTK_FILTERING_EXPORT vtkDataSetToUnstructuredGridFilter : public vtkUnstructuredGridSource
{
public:
  vtkTypeMacro(vtkDataSetToUnstructuredGridFilter,vtkUnstructuredGridSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkDataSet *input);
  vtkDataSet *GetInput();
  
protected:
  vtkDataSetToUnstructuredGridFilter();
  ~vtkDataSetToUnstructuredGridFilter();

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkDataSetToUnstructuredGridFilter(const vtkDataSetToUnstructuredGridFilter&);  // Not implemented.
  void operator=(const vtkDataSetToUnstructuredGridFilter&);  // Not implemented.
};

#endif


