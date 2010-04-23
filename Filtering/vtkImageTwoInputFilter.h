/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTwoInputFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageTwoInputFilter - Generic superclass for filters that have
// two inputs.
// .SECTION Description
// vtkImageTwoInputFilter handles two inputs.  
// It is just a subclass of vtkImageMultipleInputFilter with some
// methods that are specific to two inputs.  Although the inputs are labeled
// input1 and input2, they are stored in an array indexed starting at 0.

#ifndef __vtkImageTwoInputFilter_h
#define __vtkImageTwoInputFilter_h

#include "vtkImageMultipleInputFilter.h"

class VTK_FILTERING_EXPORT vtkImageTwoInputFilter : public vtkImageMultipleInputFilter
{
public:
  vtkTypeMacro(vtkImageTwoInputFilter,vtkImageMultipleInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the Input1 of this filter. If a ScalarType has not been set,
  // then the ScalarType of the input is used.
  virtual void SetInput1(vtkImageData *input);
  
  // Description:
  // Set the Input2 of this filter. If a ScalarType has not been set,
  // then the ScalarType of the input is used.
  virtual void SetInput2(vtkImageData *input);
  
  // Description:
  // Get the inputs to this filter.
  vtkImageData *GetInput1();
  vtkImageData *GetInput2();
  
protected:
  vtkImageTwoInputFilter();
  ~vtkImageTwoInputFilter() {};

private:
  vtkImageTwoInputFilter(const vtkImageTwoInputFilter&);  // Not implemented.
  void operator=(const vtkImageTwoInputFilter&);  // Not implemented.
};

#endif








