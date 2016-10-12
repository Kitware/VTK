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
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Called by the mapper before and after
   * rendering each prop.
   */
  virtual void BeginRenderProp();
  virtual void EndRenderProp()
    { this->vtkHardwareSelector::EndRenderProp(); }

  /**
   * Called by any vtkMapper or vtkProp subclass to render a composite-index.
   * Currently indices >= 0xffffff are not supported.
   */
  virtual void RenderCompositeIndex(unsigned int index);

  /**
   * Called by any vtkMapper or vtkProp subclass to render an attribute's id.
   */
  virtual void RenderAttributeId(vtkIdType attribid);

  /**
   * Called by any vtkMapper or subclass to render process id. This has any
   * effect when this->UseProcessIdFromData is true.
   */
  virtual void RenderProcessId(unsigned int processid);

  // we need to initialze the depth buffer
  virtual void BeginSelection();

protected:
  vtkOpenGLHardwareSelector();
  virtual ~vtkOpenGLHardwareSelector();

  void PreCapturePass(int pass) VTK_OVERRIDE;
  void PostCapturePass(int pass) VTK_OVERRIDE;

  // Called internally before and after each prop is rendered
  // for device specific configuration/preparation etc.
  virtual void BeginRenderProp(vtkRenderWindow *);
  virtual void EndRenderProp(vtkRenderWindow *);

  virtual void SavePixelBuffer(int passNo);

  // for internal state
  class vtkInternals;
  vtkInternals* Internals;

private:
  vtkOpenGLHardwareSelector(const vtkOpenGLHardwareSelector&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLHardwareSelector&) VTK_DELETE_FUNCTION;
};

#endif
