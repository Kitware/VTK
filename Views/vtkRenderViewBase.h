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
  // The interactor style associated with the render view.
  virtual void SetInteractorStyle(vtkInteractorObserver* style);
  virtual vtkInteractorObserver* GetInteractorStyle();

  //BTX
  enum
    {
    INTERACTION_MODE_2D,
    INTERACTION_MODE_3D,
    INTERACTION_MODE_UNKNOWN
    };
  //ETX
  void SetInteractionMode(int mode);
  vtkGetMacro(InteractionMode, int);

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

  // Description:
  // Whether to render on every mouse move.
  void SetRenderOnMouseMove(bool b);
  vtkGetMacro(RenderOnMouseMove, bool);
  vtkBooleanMacro(RenderOnMouseMove, bool);

protected:
  vtkRenderViewBase();
  ~vtkRenderViewBase();

  // Description:
  // Called to process events.
  // Captures StartEvent events from the renderer and calls Update().
  // This may be overridden by subclasses to process additional events.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId,
                             void* callData);

  // Description:
  // Called by the view when the renderer is about to render.
  virtual void PrepareForRendering();

  vtkRenderer* Renderer;
  vtkRenderWindow* RenderWindow;
  int InteractionMode;

private:
  vtkRenderViewBase(const vtkRenderViewBase&);  // Not implemented.
  void operator=(const vtkRenderViewBase&);  // Not implemented.

  bool RenderOnMouseMove;
};

#endif
