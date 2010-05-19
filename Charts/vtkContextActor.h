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

#ifndef __vtkContextActor_h
#define __vtkContextActor_h

#include "vtkProp.h"

class vtkContext2D;
class vtkContextScene;

class VTK_CHARTS_EXPORT vtkContextActor : public vtkProp
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkContextActor,vtkProp);

  static vtkContextActor* New();

  // Description:
  // We only render in the overlay for the context scene.
  virtual int RenderOverlay(vtkViewport *viewport);

  // Description:
  // Set the vtkContext2D for the actor.
  virtual void SetContext(vtkContext2D *context);

  // Description:
  // Set/Get the vtk2DPainter.
  vtkGetObjectMacro(Context, vtkContext2D);

  // Description:
  // Get the chart object for the Actor.
  vtkGetObjectMacro(Scene, vtkContextScene);

  // Description:
  // Set the chart object for the Actor.
  virtual void SetScene(vtkContextScene *scene);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *window);

protected:
  vtkContextActor();
  ~vtkContextActor();

  vtkContextScene *Scene;
  vtkContext2D *Context;

private:
  vtkContextActor(const vtkContextActor&);  // Not implemented.
  void operator=(const vtkContextActor&);  // Not implemented.
};

#endif
