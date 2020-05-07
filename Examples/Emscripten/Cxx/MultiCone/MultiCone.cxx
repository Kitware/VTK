/*=========================================================================
  Program:   Visualization Toolkit
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include <emscripten/emscripten.h>
#include <iostream>

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkSDL2OpenGLRenderWindow.h"
#include "vtkSDL2RenderWindowInteractor.h"

//------------------------------------------------------------------------------
// Global objects
//------------------------------------------------------------------------------

vtkRenderer* renderer = nullptr;
vtkSDL2OpenGLRenderWindow* renderWindow = nullptr;
vtkSDL2RenderWindowInteractor* renderWindowInteractor = nullptr;
vtkInteractorStyleTrackballCamera* style = nullptr;

vtkConeSource* coneSource = nullptr;
vtkPolyDataMapper* mapper = nullptr;
vtkActor* actor = nullptr;

//------------------------------------------------------------------------------

void createPipeline()
{
  // Create a renderer, render window, and interactor
  renderer = vtkRenderer::New();
  renderWindow = vtkSDL2OpenGLRenderWindow::New();
  renderWindowInteractor = vtkSDL2RenderWindowInteractor::New();
  style = vtkInteractorStyleTrackballCamera::New();

  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer);
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindowInteractor->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  // Create pipeline
  coneSource = vtkConeSource::New();
  mapper = vtkPolyDataMapper::New();
  actor = vtkActor::New();

  mapper->SetInputConnection(coneSource->GetOutputPort());
  actor->SetMapper(mapper);
  renderer->AddActor(actor);
}

//------------------------------------------------------------------------------

void deletePipeline()
{
  renderer->Delete();
  renderer = nullptr;

  renderWindow->Delete();
  renderWindow = nullptr;

  renderWindowInteractor->Delete();
  renderWindowInteractor = nullptr;

  style->Delete();
  style = nullptr;

  coneSource->Delete();
  coneSource = nullptr;

  mapper->Delete();
  mapper = nullptr;

  actor->Delete();
  actor = nullptr;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

extern "C"
{
  void stop()
  {
    std::cout << "c++::stop - begin" << std::endl;
    // TerminateApp, ExitCallback ?
    renderWindowInteractor->TerminateApp();
    deletePipeline();
    std::cout << "c++::stop - end" << std::endl;
  }

  int getConeResolution() { return coneSource->GetResolution(); }

  void setConeResolution(int resolution)
  {
    coneSource->SetResolution(resolution);
    renderWindow->Render();
  }
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
  std::cout << "Start main" << std::endl;
  createPipeline();

  // Start rendering app
  renderer->SetBackground(0.2, 0.3, 0.4);
  renderWindow->SetSize(600, 600);
  renderWindow->Render();

  // Start event loop
  // Nothing is going to execute pass that point...
  renderWindowInteractor->Start();

  return 0;
}
