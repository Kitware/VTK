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

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkHardwareSelector.h"

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLHardwareSelector : public vtkHardwareSelector
{
public:
  static vtkOpenGLHardwareSelector* New();
  vtkTypeMacro(vtkOpenGLHardwareSelector, vtkHardwareSelector);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Called by the mapper before and after
   * rendering each prop.
   */
  void BeginRenderProp() VTK_OVERRIDE;
  void EndRenderProp() VTK_OVERRIDE
    { this->vtkHardwareSelector::EndRenderProp(); }

  /**
   * Called by any vtkMapper or vtkProp subclass to render a composite-index.
   * Currently indices >= 0xffffff are not supported.
   */
  void RenderCompositeIndex(unsigned int index) VTK_OVERRIDE;

  /**
   * Called by any vtkMapper or vtkProp subclass to render an attribute's id.
   */
  void RenderAttributeId(vtkIdType attribid) VTK_OVERRIDE;

  /**
   * Called by any vtkMapper or subclass to render process id. This has any
   * effect when this->UseProcessIdFromData is true.
   */
  void RenderProcessId(unsigned int processid) VTK_OVERRIDE;

  // we need to initialze the depth buffer
  void BeginSelection() VTK_OVERRIDE;

protected:
  vtkOpenGLHardwareSelector();
  ~vtkOpenGLHardwareSelector() VTK_OVERRIDE;

  void PreCapturePass(int pass) VTK_OVERRIDE;
  void PostCapturePass(int pass) VTK_OVERRIDE;

  // Called internally before and after each prop is rendered
  // for device specific configuration/preparation etc.
  void BeginRenderProp(vtkRenderWindow *) VTK_OVERRIDE;
  void EndRenderProp(vtkRenderWindow *) VTK_OVERRIDE;

  void SavePixelBuffer(int passNo) VTK_OVERRIDE;

  // for internal state
  class vtkInternals;
  vtkInternals* Internals;

private:
  vtkOpenGLHardwareSelector(const vtkOpenGLHardwareSelector&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLHardwareSelector&) VTK_DELETE_FUNCTION;
};

#endif
