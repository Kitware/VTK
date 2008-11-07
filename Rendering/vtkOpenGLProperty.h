/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLProperty - OpenGL property
// .SECTION Description
// vtkOpenGLProperty is a concrete implementation of the abstract class 
// vtkProperty. vtkOpenGLProperty interfaces to the OpenGL rendering library.

#ifndef __vtkOpenGLProperty_h
#define __vtkOpenGLProperty_h

#include "vtkProperty.h"

class vtkOpenGLRenderer;

class VTK_RENDERING_EXPORT vtkOpenGLProperty : public vtkProperty
{
public:
  static vtkOpenGLProperty *New();
  vtkTypeRevisionMacro(vtkOpenGLProperty,vtkProperty);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Render(vtkActor *a, vtkRenderer *ren);

  // Description:
  // Implement base class method.
  void BackfaceRender(vtkActor *a, vtkRenderer *ren);

  //BTX
  // Description:
  // This method is called after the actor has been rendered.
  // Don't call this directly. This method cleans up
  // any shaders allocated.
  virtual void PostRender(vtkActor*, vtkRenderer*);
  //ETX
  
  // Description:
  // Release any graphics resources that are being consumed by this
  // property. The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *win);

protected:
  vtkOpenGLProperty() {};
  ~vtkOpenGLProperty() {};

  // Description:
  // Load OpenGL extensions for multi texturing.
  void LoadMultiTexturingExtensions(vtkRenderer* ren);
private:
  vtkOpenGLProperty(const vtkOpenGLProperty&);  // Not implemented.
  void operator=(const vtkOpenGLProperty&);  // Not implemented.
};

#endif
