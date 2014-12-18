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

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkHardwareSelector.h"

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLHardwareSelector : public vtkHardwareSelector
{
public:
  static vtkOpenGLHardwareSelector* New();
  vtkTypeMacro(vtkOpenGLHardwareSelector, vtkHardwareSelector);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called by the mapper (vtkHardwareSelectionPolyDataPainter) before and after
  // rendering each prop.
  virtual void BeginRenderProp()
    { this->vtkHardwareSelector::BeginRenderProp(); }

  virtual void EndRenderProp()
    { this->vtkHardwareSelector::EndRenderProp(); }

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
