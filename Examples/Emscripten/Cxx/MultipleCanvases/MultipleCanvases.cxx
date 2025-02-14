// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkArrowSource.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkConeSource.h"
#include "vtkCubeSource.h"
#include "vtkCylinderSource.h"
#include "vtkDiskSource.h"
#include "vtkPartitionedDataSetCollectionSource.h"
#include "vtkPlatonicSolidSource.h"
#include "vtkRegularPolygonSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkSuperquadricSource.h"
#include "vtkWebAssemblyOpenGLRenderWindow.h"
#include "vtkWebAssemblyRenderWindowInteractor.h"

#include <emscripten/emscripten.h>

#include <array>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

namespace
{
std::vector<vtkSmartPointer<vtkRenderWindowInteractor>> interactors;
}

int main(int argc, char* argv[])
{
  vtkRenderWindowInteractor::InteractorManagesTheEventLoop = false;
  if (argc < 2)
  {
    std::cerr << "Usage: MultipleCanvases <numCanvases>\n";
  }
  const int numCanvases = std::atoi(argv[1]);
  std::array<vtkSmartPointer<vtkAlgorithm>, 10> sources;
  std::size_t i = 0;
  sources[i++] = { vtk::TakeSmartPointer(vtkArrowSource::New()) };
  sources[i++] = { vtk::TakeSmartPointer(vtkConeSource::New()) };
  sources[i++] = { vtk::TakeSmartPointer(vtkCubeSource::New()) };
  sources[i++] = { vtk::TakeSmartPointer(vtkCylinderSource::New()) };
  sources[i++] = { vtk::TakeSmartPointer(vtkDiskSource::New()) };
  sources[i++] = { vtk::TakeSmartPointer(vtkPartitionedDataSetCollectionSource::New()) };
  sources[i++] = { vtk::TakeSmartPointer(vtkPlatonicSolidSource::New()) };
  sources[i++] = { vtk::TakeSmartPointer(vtkRegularPolygonSource::New()) };
  sources[i++] = { vtk::TakeSmartPointer(vtkSphereSource::New()) };
  sources[i++] = { vtk::TakeSmartPointer(vtkSuperquadricSource::New()) };

  for (int iCanvas = 0; iCanvas < numCanvases; ++iCanvas)
  {
    const std::string canvasId = "canvas" + std::to_string(iCanvas);
    const std::string canvasSelector = "#" + canvasId;
    vtkNew<vtkRenderWindow> renderWindow;
    vtkNew<vtkRenderWindowInteractor> interactor;
    vtkNew<vtkRenderer> renderer;
    vtkNew<vtkActor> actor;
    vtkNew<vtkCompositePolyDataMapper> mapper;

    auto& source = sources[iCanvas % sources.size()];
    mapper->SetInputConnection(source->GetOutputPort());
    mapper->ScalarVisibilityOff();
    actor->SetMapper(mapper);
    renderer->AddActor(actor);

    if (auto* wasmInteractor = vtkWebAssemblyRenderWindowInteractor::SafeDownCast(interactor))
    {
      wasmInteractor->SetCanvasSelector(canvasSelector.c_str());
    }
    if (auto* wasmGLWindow = vtkWebAssemblyOpenGLRenderWindow::SafeDownCast(renderWindow))
    {
      wasmGLWindow->SetCanvasSelector(canvasSelector.c_str());
    }
    renderer->SetBackground(0.3, 0.3, 0.3);
    renderWindow->AddRenderer(renderer);
    interactor->SetRenderWindow(renderWindow);
    interactors.emplace_back(interactor);
    // clang-format off
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
    MAIN_THREAD_EM_ASM({addCanvas($0, $1, $2); }, canvasId.c_str(), source->GetClassName(), iCanvas);
#pragma clang diagnostic pop
    // clang-format on
    renderer->ResetCamera();
    renderWindow->Render();
  }
  return 0;
}

extern "C"
{
  EMSCRIPTEN_KEEPALIVE void startEventLoop(int iCanvas)
  {
    if (static_cast<std::size_t>(iCanvas) < interactors.size())
    {
      interactors[iCanvas]->Start();
    }
  }

  EMSCRIPTEN_KEEPALIVE void updateSize(int iCanvas, int width, int height)
  {
    if (static_cast<std::size_t>(iCanvas) < interactors.size())
    {
      interactors[iCanvas]->UpdateSize(width, height);
    }
  }

  EMSCRIPTEN_KEEPALIVE void render(int iCanvas, int width, int height)
  {
    if (static_cast<std::size_t>(iCanvas) < interactors.size())
    {
      interactors[iCanvas]->GetRenderWindow()->Render();
    }
  }

  EMSCRIPTEN_KEEPALIVE void stopEventLoop(int iCanvas)
  {
    if (static_cast<std::size_t>(iCanvas) < interactors.size())
    {
      interactors[iCanvas]->TerminateApp();
    }
  }
}
