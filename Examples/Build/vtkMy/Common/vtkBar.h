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
/**
 * @class   vtkBar
 * @brief   Bar class for vtk
 *
 * None.
*/

#ifndef vtkBar_h
#define vtkBar_h

#include "vtkObject.h"
#include "vtkmyCommonWin32Header.h"

class VTK_MY_COMMON_EXPORT vtkBar : public vtkObject
{
public:
  static vtkBar *New();
  vtkTypeMacro(vtkBar,vtkObject);

protected:
  vtkBar() {}
  ~vtkBar() {}
private:
  vtkBar(const vtkBar&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBar&) VTK_DELETE_FUNCTION;
};

#endif
