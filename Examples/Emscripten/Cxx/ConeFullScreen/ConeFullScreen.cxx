/*=========================================================================
  Program:   Visualization Toolkit
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkSDL2OpenGLRenderWindow.h"
#include "vtkSDL2RenderWindowInteractor.h"

#include <emscripten.h>
#include <emscripten/html5.h>

//------------------------------------------------------------------------------
// Static objects
//------------------------------------------------------------------------------

static vtkSDL2OpenGLRenderWindow* renderWindow = vtkSDL2OpenGLRenderWindow::New();

//------------------------------------------------------------------------------

EM_BOOL resize_window(int eventType, const EmscriptenUiEvent* e, void* userData)
{
  renderWindow->SetSize(e->windowInnerWidth, e->windowInnerHeight);
  renderWindow->Render();
  return 0;
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
  // Create a renderer and interactor
  vtkNew<vtkRenderer> renderer;
  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkSDL2RenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  renderWindowInteractor->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  // Create pipeline
  vtkNew<vtkConeSource> coneSource;
  coneSource->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(coneSource->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Add the actors to the scene
  renderer->AddActor(actor);

  // Start rendering app
  renderer->SetBackground(0.2, 0.3, 0.4);
  renderWindow->SetSize(300, 300);

  // Attach Window Resize Callback
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, resize_window);

  // Start event loop
  renderWindowInteractor->Start();

  // Exit
  renderWindow->Delete();
  return 0;
}
