/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAppendComponents.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageAppendComponents - Collects components from two inputs into
// one output.
// .SECTION Description
// vtkImageAppendComponents takes the components from two inputs and merges
// them into one output. If Input1 has M components, and Input2 has N 
// components, the output will have M+N components with input1
// components coming first.


#ifndef __vtkImageAppendComponents_h
#define __vtkImageAppendComponents_h


#include "vtkImageMultipleInputFilter.h"

class VTK_IMAGING_EXPORT vtkImageAppendComponents : public vtkImageMultipleInputFilter
{
public:
  static vtkImageAppendComponents *New();
  vtkTypeRevisionMacro(vtkImageAppendComponents,vtkImageMultipleInputFilter);

protected:
  vtkImageAppendComponents() {};
  ~vtkImageAppendComponents() {};
  
  void ExecuteInformation(vtkImageData **inputs, vtkImageData *output);
  void ExecuteInformation(){this->vtkImageMultipleInputFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageAppendComponents(const vtkImageAppendComponents&);  // Not implemented.
  void operator=(const vtkImageAppendComponents&);  // Not implemented.
};

#endif




