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
#include "vtkmyCommonModule.h" // For export macro

class VTKMYCOMMON_EXPORT vtkBar : public vtkObject
{
public:
  static vtkBar* New();
  vtkTypeMacro(vtkBar, vtkObject);

protected:
  vtkBar() {}
  ~vtkBar() override {}

private:
  vtkBar(const vtkBar&) = delete;
  void operator=(const vtkBar&) = delete;
};

#endif
