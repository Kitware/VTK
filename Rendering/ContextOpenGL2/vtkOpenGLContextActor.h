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
/**
 * @class   vtkOpenGLContextActor
 * @brief   provides a vtkProp derived object.
 *
 * This object provides the entry point for the vtkContextScene to be rendered
 * in a vtkRenderer. Uses the RenderOverlay pass to render the 2D
 * vtkContextScene.
*/

#ifndef vtkOpenGLContextActor_h
#define vtkOpenGLContextActor_h

#include "vtkRenderingContextOpenGL2Module.h" // For export macro
#include "vtkContextActor.h"

class VTKRENDERINGCONTEXTOPENGL2_EXPORT vtkOpenGLContextActor : public vtkContextActor
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkOpenGLContextActor, vtkContextActor);

  static vtkOpenGLContextActor* New();

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *window) override;

  /**
   * We only render in the overlay for the context scene.
   */
  int RenderOverlay(vtkViewport *viewport) override;

protected:
  vtkOpenGLContextActor();
  ~vtkOpenGLContextActor() override;

  /**
   * Initialize the actor - right now we just decide which device to initialize.
   */
  void Initialize(vtkViewport* viewport) override;

private:
  vtkOpenGLContextActor(const vtkOpenGLContextActor&) = delete;
  void operator=(const vtkOpenGLContextActor&) = delete;
};

#endif
