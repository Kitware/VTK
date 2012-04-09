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
// .NAME vtkInteractorStyleSwitchBase - dummy interface class.
// .SECTION Description
// The class vtkInteractorStyleSwitchBase is here to allow the
// vtkRenderWindowInteractor to instantiate a default interactor style and
// preserve backward compatible behavior when the object factory is overridden
// and vtkInteractorStyleSwitch is returned.
//
// .SECTION See Also
// vtkInteractorStyleSwitchBase vtkRenderWindowInteractor

#ifndef __vtkInteractorStyleSwitchBase_h
#define __vtkInteractorStyleSwitchBase_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkInteractorStyle.h"

class VTKRENDERINGCORE_EXPORT vtkInteractorStyleSwitchBase
  : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleSwitchBase *New();
  vtkTypeMacro(vtkInteractorStyleSwitchBase, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkInteractorStyleSwitchBase();
  ~vtkInteractorStyleSwitchBase();

private:
  vtkInteractorStyleSwitchBase(const vtkInteractorStyleSwitchBase&); // Not implemented.
  void operator=(const vtkInteractorStyleSwitchBase&); // Not implemented.
};

#endif
