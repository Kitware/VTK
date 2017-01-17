/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleSwitchBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInteractorStyleSwitchBase
 * @brief   dummy interface class.
 *
 * The class vtkInteractorStyleSwitchBase is here to allow the
 * vtkRenderWindowInteractor to instantiate a default interactor style and
 * preserve backward compatible behavior when the object factory is overridden
 * and vtkInteractorStyleSwitch is returned.
 *
 * @sa
 * vtkInteractorStyleSwitchBase vtkRenderWindowInteractor
*/

#ifndef vtkInteractorStyleSwitchBase_h
#define vtkInteractorStyleSwitchBase_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkInteractorStyle.h"

class VTKRENDERINGCORE_EXPORT vtkInteractorStyleSwitchBase
  : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleSwitchBase *New();
  vtkTypeMacro(vtkInteractorStyleSwitchBase, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkRenderWindowInteractor* GetInteractor() VTK_OVERRIDE;

protected:
  vtkInteractorStyleSwitchBase();
  ~vtkInteractorStyleSwitchBase() VTK_OVERRIDE;

private:
  vtkInteractorStyleSwitchBase(const vtkInteractorStyleSwitchBase&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInteractorStyleSwitchBase&) VTK_DELETE_FUNCTION;
};

#endif
