/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPReflectionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPReflectionFilter - parallel version of vtkReflectionFilter
// .SECTION Description
// vtkPReflectionFilter is a parallel version of vtkReflectionFilter which takes
// into consideration the full dataset bounds for performing the reflection.
#ifndef __vtkPReflectionFilter_h
#define __vtkPReflectionFilter_h

#include "vtkReflectionFilter.h"

class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkPReflectionFilter : public vtkReflectionFilter
{
public:
  static vtkPReflectionFilter* New();
  vtkTypeMacro(vtkPReflectionFilter, vtkReflectionFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the parallel controller.
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro (Controller, vtkMultiProcessController);

//BTX
protected:
  vtkPReflectionFilter();
  ~vtkPReflectionFilter();

  // Description:
  // Internal method to compute bounds.
  virtual int ComputeBounds(vtkDataObject* input, double bounds[6]);

  vtkMultiProcessController* Controller;
private:
  vtkPReflectionFilter(const vtkPReflectionFilter&); // Not implemented.
  void operator=(const vtkPReflectionFilter&); // Not implemented.
//ETX
};

#endif


