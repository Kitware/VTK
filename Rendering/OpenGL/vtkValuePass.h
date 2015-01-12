/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValuePass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkValuePass - Render opaque objects with the vtkValuePainter
// .SECTION Description
// This is a render pass draws polygonal data with the vtkValuePainter.
//
// .SECTION See Also
// vtkValuePainter, vtkValuePass

#ifndef vtkValuePass_h
#define vtkValuePass_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkOpaquePass.h"

class VTKRENDERINGOPENGL_EXPORT vtkValuePass : public vtkOpaquePass
{
public:
  static vtkValuePass *New();
  vtkTypeMacro(vtkValuePass,vtkOpaquePass);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the array to be drawn.
  void SetInputArrayToProcess(int fieldAssociation, const char *name);
  void SetInputArrayToProcess(int fieldAssociation, int fieldAttributeType);
  void SetInputComponentToProcess(int comp);
  void SetScalarRange(double min, double max);

  // Description:
  // Render.
  virtual void Render(const vtkRenderState *s);

 protected:
  // Description:
  // Default constructor.
  vtkValuePass();

  // Description:
  // Destructor.
  virtual ~vtkValuePass();

 private:
  vtkValuePass(const vtkValuePass&);  // Not implemented.
  void operator=(const vtkValuePass&);  // Not implemented.

  class vtkInternals;
  vtkInternals *Internals;
};

#endif
