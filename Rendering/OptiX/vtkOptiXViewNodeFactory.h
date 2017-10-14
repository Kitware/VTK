/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXViewNodeFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOptiXViewNodeFactory
 * @brief   matches vtk rendering classes to
 * specific OptiX ViewNode classes
 *
 * Ensures that vtkOptiXPass makes OptiX specific translator instances
 * for every VTK rendering pipeline class instance it encounters.
*/

#ifndef vtkOptiXViewNodeFactory_h
#define vtkOptiXViewNodeFactory_h

#include "vtkRenderingOptiXModule.h" // For export macro
#include "vtkViewNodeFactory.h"

class VTKRENDERINGOPTIX_EXPORT vtkOptiXViewNodeFactory :
  public vtkViewNodeFactory
{
public:
  static vtkOptiXViewNodeFactory* New();
  vtkTypeMacro(vtkOptiXViewNodeFactory, vtkViewNodeFactory);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkOptiXViewNodeFactory();
  ~vtkOptiXViewNodeFactory();

private:
  vtkOptiXViewNodeFactory(const vtkOptiXViewNodeFactory&) = delete;
  void operator=(const vtkOptiXViewNodeFactory&) = delete;
};

#endif
