/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLHardwareSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLHardwareSelector
 * @brief   implements the device specific code of
 *  vtkOpenGLHardwareSelector.
 *
 *
 * Implements the device specific code of vtkOpenGLHardwareSelector.
 *
 * @sa
 * vtkHardwareSelector
*/

#ifndef vtkOpenGLHardwareSelector_h
#define vtkOpenGLHardwareSelector_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkHardwareSelector.h"

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLHardwareSelector : public vtkHardwareSelector
{
public:
  static vtkOpenGLHardwareSelector* New();
  vtkTypeMacro(vtkOpenGLHardwareSelector, vtkHardwareSelector);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Called by the mapper (vtkHardwareSelectionPolyDataPainter) before and after
   * rendering each prop.
   */
  void BeginRenderProp() VTK_OVERRIDE
    { this->vtkHardwareSelector::BeginRenderProp(); }

  void EndRenderProp() VTK_OVERRIDE
    { this->vtkHardwareSelector::EndRenderProp(); }

protected:
  vtkOpenGLHardwareSelector();
  ~vtkOpenGLHardwareSelector() VTK_OVERRIDE;

  // Called internally before and after each prop is rendered
  // for device specific configuration/preparation etc.
  void BeginRenderProp(vtkRenderWindow *) VTK_OVERRIDE;
  void EndRenderProp(vtkRenderWindow *) VTK_OVERRIDE;

  // for internal state
  class vtkInternals;
  vtkInternals* Internals;

private:
  vtkOpenGLHardwareSelector(const vtkOpenGLHardwareSelector&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLHardwareSelector&) VTK_DELETE_FUNCTION;
};

#endif
