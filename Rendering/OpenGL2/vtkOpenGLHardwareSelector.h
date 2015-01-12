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
// .NAME vtkOpenGLHardwareSelector - implements the device specific code of
//  vtkOpenGLHardwareSelector.
//
// .SECTION Description
// Implements the device specific code of vtkOpenGLHardwareSelector.
//
// .SECTION See Also
// vtkHardwareSelector

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

  // Description:
  // Called by the mapper before and after
  // rendering each prop.
  virtual void BeginRenderProp();
  virtual void EndRenderProp()
    { this->vtkHardwareSelector::EndRenderProp(); }

  // Description:
  // Called by any vtkMapper or vtkProp subclass to render a composite-index.
  // Currently indices >= 0xffffff are not supported.
  virtual void RenderCompositeIndex(unsigned int index);

  // Description:
  // Called by any vtkMapper or vtkProp subclass to render an attribute's id.
  virtual void RenderAttributeId(vtkIdType attribid);

  // Description:
  // Called by any vtkMapper or subclass to render process id. This has any
  // effect when this->UseProcessIdFromData is true.
  virtual void RenderProcessId(unsigned int processid);

protected:
  vtkOpenGLHardwareSelector();
  virtual ~vtkOpenGLHardwareSelector();

  // Called internally before and after each prop is rendered
  // for device specific configuration/preparation etc.
  virtual void BeginRenderProp(vtkRenderWindow *);
  virtual void EndRenderProp(vtkRenderWindow *);

  // for internal state
  class vtkInternals;
  vtkInternals* Internals;

private:
  vtkOpenGLHardwareSelector(const vtkOpenGLHardwareSelector&); // Not implemented.
  void operator=(const vtkOpenGLHardwareSelector&); // Not implemented.
};

#endif
