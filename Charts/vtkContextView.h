/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkContextView - provides a view of the vtkContextScene.
//
// .SECTION Description
// This class is derived from vtkRenderView and provides a view of a
// vtkContextScene, with a default interactor style, renderer etc.

#ifndef __vtkContextView_h
#define __vtkContextView_h

#include "vtkRenderView.h"

class vtkContext2D;
class vtkContextScene;
class vtkRenderWindowInteractor;

class VTK_CHARTS_EXPORT vtkContextView : public vtkRenderView
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkContextView,vtkRenderView);

  static vtkContextView* New();

  // Description:
  // Set the vtkContext2D for the view.
  virtual void SetContext(vtkContext2D *context);

  // Description:
  // Get the vtkContext2D for the view.
  vtkGetObjectMacro(Context, vtkContext2D);

  // Description:
  // Set the interaction mode, defaults to 2D here.
  virtual void SetInteractionMode(int mode);

  // Description:
  // Get the scene of the view.
  vtkGetObjectMacro(Scene, vtkContextScene);

  // Description:
  // Set the scene object for the view.
  virtual void SetScene(vtkContextScene *scene);

  // Description:
  // Updates the representations, then calls Render() on the render window
  // associated with this view.
  virtual void Render();

protected:
  vtkContextView();
  ~vtkContextView();

  vtkContextScene *Scene;
  vtkContext2D *Context;

private:
  vtkContextView(const vtkContextView&);  // Not implemented.
  void operator=(const vtkContextView&);  // Not implemented.
};

#endif
