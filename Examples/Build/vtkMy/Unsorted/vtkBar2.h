/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBar2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBar2
 * @brief   Bar2 class for vtk
 *
 * None.
 */

#ifndef vtkBar2_h
#define vtkBar2_h

#include "vtkObject.h"
#include "vtkmyUnsortedModule.h" // For export macro

class VTKMYUNSORTED_EXPORT vtkBar2 : public vtkObject
{
public:
  static vtkBar2* New();
  vtkTypeMacro(vtkBar2, vtkObject);

protected:
  vtkBar2() {}
  ~vtkBar2() override {}

private:
  vtkBar2(const vtkBar2&) = delete;
  void operator=(const vtkBar2&) = delete;
};

#endif
