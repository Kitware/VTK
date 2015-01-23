/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkContextActor - provides a vtkProp derived object.
// .SECTION Description
// This object provides the entry point for the vtkContextScene to be rendered
// in a vtkRenderer. Uses the RenderOverlay pass to render the 2D
// vtkContextScene.

#ifndef vtkContextActor_h
#define vtkContextActor_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkProp.h"
#include "vtkNew.h"          // For ivars
#include "vtkSmartPointer.h" // For ivars

class vtkContext2D;
class vtkContext3D;
class vtkContextScene;

class VTKRENDERINGCONTEXT2D_EXPORT vtkContextActor : public vtkProp
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkContextActor,vtkProp);

  static vtkContextActor* New();

  // Description:
  // We only render in the overlay for the context scene.
  virtual int RenderOverlay(vtkViewport *viewport);

  // Description:
  // Get the vtkContext2D for the actor.
  vtkGetNewMacro(Context, vtkContext2D);

  // Description:
  // Get the chart object for the actor.
  vtkContextScene * GetScene();

  // Description:
  // Set the scene for the actor.
  void SetScene(vtkContextScene *scene);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *window);

protected:
  vtkContextActor();
  ~vtkContextActor();

  // Description:
  // Initialize the actor - right now we just decide which device to initialize.
  virtual void Initialize(vtkViewport* viewport);

  vtkSmartPointer<vtkContextScene> Scene;
  vtkNew<vtkContext2D> Context;
  vtkNew<vtkContext3D> Context3D;
  bool Initialized;

private:
  vtkContextActor(const vtkContextActor&);  // Not implemented.
  void operator=(const vtkContextActor&);  // Not implemented.
};

#endif
