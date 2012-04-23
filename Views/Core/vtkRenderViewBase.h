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
//
// In order to use the view with a QVTKWidget the following code is required
// to ensure the interactor and render window are initialized properly.
// \code
// QVTKWidget *widget = new QVTKWidget;
// vtkContextView *view = vtkContextView::New();
// view->SetInteractor(widget->GetInteractor());
// widget->SetRenderWindow(view->GetRenderWindow());
// \endcode

#ifndef __vtkRenderViewBase_h
#define __vtkRenderViewBase_h

#include "vtkViewsCoreModule.h" // For export macro
#include "vtkView.h"
#include "vtkSmartPointer.h" // For SP ivars

class vtkInteractorObserver;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;

class VTKVIEWSCORE_EXPORT vtkRenderViewBase : public vtkView
{
public:
  static vtkRenderViewBase* New();
  vtkTypeMacro(vtkRenderViewBase, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Gets the renderer for this view.
  virtual vtkRenderer* GetRenderer();

  // Sets the renderer for this view.
  virtual void SetRenderer(vtkRenderer* ren);

  // Description:
  // Get a handle to the render window.
  virtual vtkRenderWindow* GetRenderWindow();

  // Description:
  // Set the render window for this view. Note that this requires special
  // handling in order to do correctly - see the notes in the detailed
  // description of vtkRenderViewBase.
  virtual void SetRenderWindow(vtkRenderWindow *win);

  // Description:
  // The render window interactor. Note that this requires special
  // handling in order to do correctly - see the notes in the detailed
  // description of vtkRenderViewBase.
  virtual vtkRenderWindowInteractor* GetInteractor();
  virtual void SetInteractor(vtkRenderWindowInteractor*);

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

  vtkSmartPointer<vtkRenderer> Renderer;
  vtkSmartPointer<vtkRenderWindow> RenderWindow;

private:
  vtkRenderViewBase(const vtkRenderViewBase&);  // Not implemented.
  void operator=(const vtkRenderViewBase&);  // Not implemented.
};

#endif
