/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToUnstructuredGridFilter.h
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

class VTK_FILTERING_EXPORT vtkDataSetToUnstructuredGridFilter : public vtkUnstructuredGridSource
{
public:
  vtkTypeRevisionMacro(vtkDataSetToUnstructuredGridFilter,vtkUnstructuredGridSource);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkDataSet *input);
  vtkDataSet *GetInput();
  
protected:
  vtkDataSetToUnstructuredGridFilter() {this->NumberOfRequiredInputs = 1;};
  ~vtkDataSetToUnstructuredGridFilter() {};
  

private:
  vtkDataSetToUnstructuredGridFilter(const vtkDataSetToUnstructuredGridFilter&);  // Not implemented.
  void operator=(const vtkDataSetToUnstructuredGridFilter&);  // Not implemented.
};

#endif


