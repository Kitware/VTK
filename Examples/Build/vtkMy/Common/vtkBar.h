/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBar.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBar - Bar class for vtk
// .SECTION Description
// None.

#ifndef __vtkBar_h
#define __vtkBar_h

#include "vtkObject.h"
#include "vtkmyCommonWin32Header.h"

class VTK_MY_COMMON_EXPORT vtkBar : public vtkObject
{
public:
  static vtkBar *New();
  vtkTypeMacro(vtkBar,vtkObject);

protected:
  vtkBar() {};
  ~vtkBar() {};
private:
  vtkBar(const vtkBar&);  // Not implemented.
  void operator=(const vtkBar&);  // Not implemented.
};

#endif
