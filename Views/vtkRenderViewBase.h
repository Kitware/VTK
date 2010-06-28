/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderViewBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkRenderViewBase - A base view containing a renderer.
//
// .SECTION Description
// vtkRenderViewBase is a view which contains a vtkRenderer.  You may add
// vtkActors directly to the renderer.
//
// This class is also the parent class for any more specialized view which uses
// a renderer.

#ifndef __vtkRenderViewBase_h
#define __vtkRenderViewBase_h

#include "vtkView.h"
#include "vtkSmartPointer.h" // For SP ivars

class vtkInteractorObserver;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;

class VTK_VIEWS_EXPORT vtkRenderViewBase : public vtkView
{
public:
  static vtkRenderViewBase* New();
  vtkTypeMacro(vtkRenderViewBase, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Gets the renderer for this view.
  vtkGetObjectMacro(Renderer, vtkRenderer);

  // Description:
  // Get a handle to the render window.
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  virtual void SetRenderWindow(vtkRenderWindow *win);

  // Description:
  // The render window interactor.
  virtual vtkRenderWindowInteractor* GetInteractor();
  virtual void SetInteractor(vtkRenderWindowInteractor *interactor);

  // Description:
  // Updates the representations, then calls Render() on the render window
  // associated with this view.
  virtual void Render();

  // Description:
  // Updates the representations, then calls ResetCamera() on the renderer
  // associated with this view.
  virtual void ResetCamera();

  // Description:
  // Updates the representations, then calls ResetCameraClippingRange() on the
  // renderer associated with this view.
  virtual void ResetCameraClippingRange();

protected:
  vtkRenderViewBase();
  ~vtkRenderViewBase();

  // Description:
  // Called by the view when the renderer is about to render.
  virtual void PrepareForRendering();

  vtkRenderer* Renderer;
  vtkRenderWindow* RenderWindow;

private:
  vtkRenderViewBase(const vtkRenderViewBase&);  // Not implemented.
  void operator=(const vtkRenderViewBase&);  // Not implemented.
};

#endif
