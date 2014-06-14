/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGL2HardwareSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGL2HardwareSelector - implements the device specific code of
//  vtkOpenGL2HardwareSelector.
//
// .SECTION Description
// Implements the device specific code of vtkOpenGL2HardwareSelector.
//
// .SECTION See Also
// vtkHardwareSelector

#ifndef __vtkOpenGL2HardwareSelector_h
#define __vtkOpenGL2HardwareSelector_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkHardwareSelector.h"

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGL2HardwareSelector : public vtkHardwareSelector
{
public:
  static vtkOpenGL2HardwareSelector* New();
  vtkTypeMacro(vtkOpenGL2HardwareSelector, vtkHardwareSelector);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called by the mapper before and after
  // rendering each prop.
  virtual void BeginRenderProp();
  //virtual void EndRenderProp();

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
  vtkOpenGL2HardwareSelector();
  virtual ~vtkOpenGL2HardwareSelector();

  // Called internally before and after each prop is rendered
  // for device specific configuration/preparation etc.
  virtual void BeginRenderProp(vtkRenderWindow *);
  virtual void EndRenderProp(vtkRenderWindow *);

  // for internal state
  class vtkInternals;
  vtkInternals* Internals;

private:
  vtkOpenGL2HardwareSelector(const vtkOpenGL2HardwareSelector&); // Not implemented.
  void operator=(const vtkOpenGL2HardwareSelector&); // Not implemented.
};

#endif
