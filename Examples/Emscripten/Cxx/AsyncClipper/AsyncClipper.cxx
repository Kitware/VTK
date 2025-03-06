// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellType.h>
#include <vtkCellTypeSource.h>
#include <vtkClipPolyData.h>
#include <vtkCommand.h>
#include <vtkDataSetMapper.h>
#include <vtkImplicitPlaneRepresentation.h>
#include <vtkImplicitPlaneWidget2.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMapper.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTableBasedClipDataSet.h>
#include <vtkUnstructuredGrid.h>

#include <emscripten/proxying.h>
#include <emscripten/threading.h>

#include <algorithm>
#include <array>

namespace
{
// Callback for the interaction
class vtkIPWCallback : public vtkCommand
{
public:
  static vtkIPWCallback* New() { return new vtkIPWCallback; }

  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    auto* planeWidget = reinterpret_cast<vtkImplicitPlaneWidget2*>(caller);
    auto* rep = reinterpret_cast<vtkImplicitPlaneRepresentation*>(planeWidget->GetRepresentation());
    rep->GetPlane(this->plane);
  }

  vtkIPWCallback() = default;

  vtkPlane* plane{ nullptr };
};

vtkSmartPointer<vtkTableBasedClipDataSet> clipper;

} // namespace

extern "C"
{
  EMSCRIPTEN_KEEPALIVE void AbortClip()
  {
    // set abort flag.
    clipper->SetAbortExecuteAndUpdateTime();
  }
  EMSCRIPTEN_KEEPALIVE void ResetAbortFlagForClip()
  {
    // set abort flag.
    clipper->SetAbortExecute(false);
  }
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------
int main(int argc, char** argv)
{
  std::array<int, 3> ndims;
  std::transform(&argv[1], argv + argc, ndims.begin(), std::atoi);
  std::cout << "Generating " << ndims[0] << 'x' << ndims[1] << 'x' << ndims[2]
            << " block of hexahedra\n";

  // Create pipeline
  vtkNew<vtkCellTypeSource> ugridSource;
  ugridSource->SetCellType(VTK_HEXAHEDRON);
  ugridSource->SetBlocksDimensions(ndims.data());
  ugridSource->Update();

  vtkNew<vtkDataSetMapper> ugridMapper;
  ugridMapper->SetInputConnection(ugridSource->GetOutputPort());

  vtkNew<vtkActor> ugridActor;
  ugridActor->SetMapper(ugridMapper);
  ugridActor->GetProperty()->SetOpacity(0.3);

  clipper = vtk::TakeSmartPointer(vtkTableBasedClipDataSet::New());
  vtkNew<vtkPlane> plane;
  auto* ugrid = ugridSource->GetOutput();
  std::array<double, 6> bounds;
  ugrid->GetBounds(bounds.data());
  std::array<double, 3> origin;
  for (int i = 0; i < 3; ++i)
  {
    origin[i] = 0.5 * (bounds[i * 2] + bounds[i * 2 + 1]);
  }
  plane->SetNormal(1, 0, 0);
  plane->SetOrigin(origin.data());
  clipper->SetClipFunction(plane);
  clipper->SetInputData(ugrid);

  // Render
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkDataSetMapper> clippedMapper;
  clippedMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(1, 1);
  clippedMapper->SetInputConnection(clipper->GetOutputPort());

  vtkNew<vtkActor> clippedActor;
  clippedActor->SetMapper(clippedMapper);
  clippedActor->GetProperty()->SetEdgeVisibility(true);
  clippedActor->GetProperty()->SetEdgeColor(0, 0, 1);

  // Create a renderer, render window, and interactor
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  renderWindowInteractor->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  // Add the actors to the scene
  renderer->AddActor(ugridActor);
  renderer->AddActor(clippedActor);

  // The callback will do the work.
  vtkNew<vtkIPWCallback> myCallback;
  myCallback->plane = plane;

  vtkNew<vtkImplicitPlaneRepresentation> rep;
  rep->SetPlaceFactor(1.25); // This must be set prior to placing the widget.
  rep->PlaceWidget(bounds.data());
  rep->SetPlane(plane);
  rep->SetDrawOutline(false);

  vtkNew<vtkImplicitPlaneWidget2> planeWidget;
  planeWidget->SetInteractor(renderWindowInteractor);
  planeWidget->SetRepresentation(rep);
  planeWidget->AddObserver(vtkCommand::InteractionEvent, myCallback);

  renderer->GetActiveCamera()->Azimuth(-60);
  renderer->GetActiveCamera()->Elevation(30);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(0.75);

  // Render and interact.
  renderWindowInteractor->Initialize();
  renderWindow->Render();
  planeWidget->On();

  // clang-format off
  // Trigger abort when mouse move occurs followed by mouse down.
  // Reset the abort flag after mouse button is released so that the clipper
  // can execute with the new plane orientation.
  MAIN_THREAD_EM_ASM({
    var mouseDown = false;
    let canvas = document.getElementById("canvas");
    canvas.addEventListener('mousedown', (e) => {
      mouseDown = true;
      Module._AbortClip();
      e.preventDefault();
    });
    canvas.addEventListener('mousemove', () => {
      if (mouseDown)
      {
        Module._AbortClip();
      }
    });
    canvas.addEventListener('mouseup', (e) => {
      mouseDown = false;
      Module._ResetAbortFlagForClip();
    });
    // Resize the canvas to fill up window space.
    setTimeout(() => window.dispatchEvent(new Event("resize")), 3000)
  });
  // clang-format on

  // Start event loop
  renderWindowInteractor->Start();

  return 0;
}
