/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAppendComponents.h
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

#ifndef VTK_REMOVE_LEGACY_CODE
  // Description:
  // Do not use these: They are for legacy compatibility back when this was a
  // two input filter.
  virtual void SetInput1(vtkImageData *input)
    {VTK_LEGACY_METHOD(SetInput,"3.2"); this->SetInput(0, input);}
  virtual void SetInput2(vtkImageData *input)
    {VTK_LEGACY_METHOD(SetInput,"3.2"); this->SetInput(1, input);}
#endif
  
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




