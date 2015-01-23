/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLContextActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLContextActor - provides a vtkProp derived object.
// .SECTION Description
// This object provides the entry point for the vtkContextScene to be rendered
// in a vtkRenderer. Uses the RenderOverlay pass to render the 2D
// vtkContextScene.

#ifndef vtkOpenGLContextActor_h
#define vtkOpenGLContextActor_h

#include "vtkRenderingContextOpenGL2Module.h" // For export macro
#include "vtkContextActor.h"

class VTKRENDERINGCONTEXTOPENGL2_EXPORT vtkOpenGLContextActor : public vtkContextActor
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkOpenGLContextActor, vtkContextActor);

  static vtkOpenGLContextActor* New();

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *window);

protected:
  vtkOpenGLContextActor();
  ~vtkOpenGLContextActor();

  // Description:
  // Initialize the actor - right now we just decide which device to initialize.
  void Initialize(vtkViewport* viewport);

private:
  vtkOpenGLContextActor(const vtkOpenGLContextActor&);  // Not implemented.
  void operator=(const vtkOpenGLContextActor&);  // Not implemented.
};

#endif
