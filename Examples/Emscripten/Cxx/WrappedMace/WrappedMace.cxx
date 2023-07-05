// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkAlgorithmOutput.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSDL2RenderWindowInteractor.h"
#include "vtkSphereSource.h"

#include <emscripten/bind.h>

#define vtkAddDestructor(cname)                                                                    \
  template <>                                                                                      \
  void raw_destructor<cname>(cname * ptr)                                                          \
  {                                                                                                \
    ptr->Delete();                                                                                 \
  }

// Since VTK destructors are private have to override all
// wrapped classes here
namespace emscripten
{
namespace internal
{
vtkAddDestructor(vtkActor);
vtkAddDestructor(vtkAlgorithm);
vtkAddDestructor(vtkAlgorithmOutput);
vtkAddDestructor(vtkConeSource);
vtkAddDestructor(vtkGlyph3D);
vtkAddDestructor(vtkMapper);
vtkAddDestructor(vtkPolyDataMapper);
vtkAddDestructor(vtkProp);
vtkAddDestructor(vtkRenderer);
vtkAddDestructor(vtkRenderWindow);
vtkAddDestructor(vtkRenderWindowInteractor);
vtkAddDestructor(vtkSDL2RenderWindowInteractor);
vtkAddDestructor(vtkSphereSource);
vtkAddDestructor(vtkViewport);
}
}

EMSCRIPTEN_BINDINGS(webtest)
{
  // vtkActor ---------------------------------------------------------
  emscripten::class_<vtkActor, emscripten::base<vtkProp>>("vtkActor")
    .constructor(&vtkActor::New, emscripten::allow_raw_pointers())
    .function("SetMapper", &vtkActor::SetMapper, emscripten::allow_raw_pointers());

  // vtkAlgorithm -----------------------------------------------------
  emscripten::class_<vtkAlgorithm>("vtkAlgorithm")
    .function("GetOutputPort",
      emscripten::select_overload<vtkAlgorithmOutput*(vtkAlgorithm&)>(
        [](vtkAlgorithm& self) { return self.vtkAlgorithm::GetOutputPort(); }),
      emscripten::allow_raw_pointers())
    .function("SetInputConnection",
      emscripten::select_overload<void(vtkAlgorithm&, vtkAlgorithmOutput*)>(
        [](vtkAlgorithm& self, vtkAlgorithmOutput* ptr) {
          self.vtkAlgorithm::SetInputConnection(ptr);
        }),
      emscripten::allow_raw_pointers());

  // vtkAlgorithmOutput -----------------------------------------------
  emscripten::class_<vtkAlgorithmOutput>("vtkAlgorithmOutput");

  // vtkConeSource ----------------------------------------------------
  emscripten::class_<vtkConeSource, emscripten::base<vtkAlgorithm>>("vtkConeSource")
    .constructor(&vtkConeSource::New, emscripten::allow_raw_pointers())
    .function("SetResolution", &vtkConeSource::SetResolution);

  // vtkGlyph3D -------------------------------------------------------
  emscripten::class_<vtkGlyph3D, emscripten::base<vtkAlgorithm>>("vtkGlyph3D")
    .constructor(&vtkGlyph3D::New, emscripten::allow_raw_pointers())
    .function("SetVectorModeToUseNormal", &vtkGlyph3D::SetVectorModeToUseNormal)
    .function("SetScaleModeToScaleByVector", &vtkGlyph3D::SetScaleModeToScaleByVector)
    .function("SetScaleFactor", &vtkGlyph3D::SetScaleFactor)
    .function("SetSourceConnection",
      emscripten::select_overload<void(vtkGlyph3D&, vtkAlgorithmOutput*)>(
        [](vtkGlyph3D& self, vtkAlgorithmOutput* ptr) {
          self.vtkGlyph3D::SetSourceConnection(ptr);
        }),
      emscripten::allow_raw_pointers());

  // vtkMapper --------------------------------------------------------
  emscripten::class_<vtkMapper, emscripten::base<vtkAlgorithm>>("vtkMapper");

  // vtkPolyDataMapper ------------------------------------------------
  emscripten::class_<vtkPolyDataMapper, emscripten::base<vtkMapper>>("vtkPolyDataMapper")
    .constructor(&vtkPolyDataMapper::New, emscripten::allow_raw_pointers());

  // vtkProp ----------------------------------------------------------
  emscripten::class_<vtkProp>("vtkProp");

  // vtkRenderer ------------------------------------------------------
  emscripten::class_<vtkRenderer, emscripten::base<vtkViewport>>("vtkRenderer")
    .constructor(&vtkRenderer::New, emscripten::allow_raw_pointers())
    .function("AddActor", &vtkRenderer::AddActor, emscripten::allow_raw_pointers());

  // vtkRenderWindow --------------------------------------------------
  emscripten::class_<vtkRenderWindow>("vtkRenderWindow")
    .constructor(&vtkRenderWindow::New, emscripten::allow_raw_pointers())
    .function("SetMultiSamples", &vtkRenderWindow::SetMultiSamples)
    .function("Render", &vtkRenderWindow::Render)
    .function("AddRenderer", &vtkRenderWindow::AddRenderer, emscripten::allow_raw_pointers())
    .function("SetSize",
      emscripten::select_overload<void(vtkRenderWindow&, int, int)>(
        [](vtkRenderWindow& self, int w, int h) { self.SetSize(w, h); }));

  // vtkRenderWindowInteractor ----------------------------------------
  emscripten::class_<vtkRenderWindowInteractor>("vtkRenderWindowInteractor")
    .function("Start", &vtkRenderWindowInteractor::Start)
    .function("SetRenderWindow", &vtkRenderWindowInteractor::SetRenderWindow,
      emscripten::allow_raw_pointers());

  // vtkSDL2RenderWindowInteractor ------------------------------------
  emscripten::class_<vtkSDL2RenderWindowInteractor, emscripten::base<vtkRenderWindowInteractor>>(
    "vtkSDL2RenderWindowInteractor")
    .constructor(&vtkSDL2RenderWindowInteractor::New, emscripten::allow_raw_pointers())
    .function("AddEventHandler", &vtkSDL2RenderWindowInteractor::AddEventHandler);

  // vtkSphereSource -------------------------------------------------
  emscripten::class_<vtkSphereSource, emscripten::base<vtkAlgorithm>>("vtkSphereSource")
    .constructor(&vtkSphereSource::New, emscripten::allow_raw_pointers())
    .function("SetThetaResolution", &vtkSphereSource::SetThetaResolution)
    .function("SetPhiResolution", &vtkSphereSource::SetPhiResolution);

  // vtkViewport ------------------------------------------------------
  emscripten::class_<vtkViewport>("vtkViewport")
    .function("SetBackground",
      emscripten::select_overload<void(vtkViewport&, double, double, double)>(
        [](vtkViewport& self, double r, double g, double b) {
          self.vtkViewport::SetBackground(r, g, b);
        }));
}
