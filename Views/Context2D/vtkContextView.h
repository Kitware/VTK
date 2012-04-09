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
// This class is derived from vtkRenderViewBase and provides a view of a
// vtkContextScene, with a default interactor style, renderer etc. It is
// the simplest way to create a vtkRenderWindow and display a 2D scene inside
// of it.
//
// By default the scene has a white background.

#ifndef __vtkContextView_h
#define __vtkContextView_h

#include "vtkViewsContext2DModule.h" // For export macro
#include "vtkRenderViewBase.h"
#include "vtkSmartPointer.h" // Needed for SP ivars

class vtkContext2D;
class vtkContextScene;

class VTKVIEWSCONTEXT2D_EXPORT vtkContextView : public vtkRenderViewBase
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkContextView, vtkRenderViewBase);

  static vtkContextView* New();

  // Description:
  // Set the vtkContext2D for the view.
  virtual void SetContext(vtkContext2D *context);

  // Description:
  // Get the vtkContext2D for the view.
  virtual vtkContext2D* GetContext();

  // Description:
  // Set the scene object for the view.
  virtual void SetScene(vtkContextScene *scene);

  // Description:
  // Get the scene of the view.
  virtual vtkContextScene* GetScene();

protected:
  vtkContextView();
  ~vtkContextView();

  vtkSmartPointer<vtkContextScene> Scene;
  vtkSmartPointer<vtkContext2D> Context;

private:
  vtkContextView(const vtkContextView&);  // Not implemented.
  void operator=(const vtkContextView&);  // Not implemented.
};

#endif
