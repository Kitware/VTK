/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayViewNodeFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayViewNodeFactory
 * @brief   matches vtk rendering classes to
 * specific ospray ViewNode classes
 *
 * Ensures that vtkOSPRayPass makes ospray specific translator instances
 * for every VTK rendering pipeline class instance it encounters.
*/

#ifndef vtkOSPRayViewNodeFactory_h
#define vtkOSPRayViewNodeFactory_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkViewNodeFactory.h"

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayViewNodeFactory :
  public vtkViewNodeFactory
{
public:
  static vtkOSPRayViewNodeFactory* New();
  vtkTypeMacro(vtkOSPRayViewNodeFactory, vtkViewNodeFactory);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkOSPRayViewNodeFactory();
  ~vtkOSPRayViewNodeFactory();

private:
  vtkOSPRayViewNodeFactory(const vtkOSPRayViewNodeFactory&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOSPRayViewNodeFactory&) VTK_DELETE_FUNCTION;
};

#endif
