// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, var) vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

namespace
{
void SetupActorWithEdgeOpacity(vtkActor* actor, vtkPolyDataMapper* mapper, int* pos, double opacity)
{
  actor->SetMapper(mapper);
  actor->SetPosition(pos[0], pos[1], pos[2]);
  actor->GetProperty()->EdgeVisibilityOn();
  actor->GetProperty()->SetEdgeColor(0.0, 0.0, 0.5);
  actor->GetProperty()->SetEdgeOpacity(opacity);
}
}

int TestEdgeOpacity(int argc, char* argv[])
{
  // to make sure that wireframe will be visible
  vtkMapper::SetResolveCoincidentTopologyToShiftZBuffer();
  vtkMapper::SetResolveCoincidentTopologyZShift(0.1);

  VTK_CREATE(vtkSphereSource, sphere);
  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(sphere->GetOutputPort());

  VTK_CREATE(vtkActor, actor1);
  int pos[3] = { 0, 0, 0 };
  double opacity = 0.33;
  ::SetupActorWithEdgeOpacity(actor1, mapper, pos, opacity);

  VTK_CREATE(vtkActor, actor2);
  pos[0] = 1.5;
  opacity = 0.66;
  ::SetupActorWithEdgeOpacity(actor2, mapper, pos, opacity);

  VTK_CREATE(vtkActor, actor3);
  pos[0] = 2;
  opacity = 1.0;
  ::SetupActorWithEdgeOpacity(actor3, mapper, pos, opacity);

  VTK_CREATE(vtkRenderer, renderer);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->AddActor(actor3);
  renderer->ResetCamera();

  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->AddRenderer(renderer);
  renwin->SetSize(250, 250);
  renwin->SetMultiSamples(0);

  int retVal = vtkRegressionTestImage(renwin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    VTK_CREATE(vtkRenderWindowInteractor, iren);
    iren->SetRenderWindow(renwin);
    iren->Initialize();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  return (retVal == vtkRegressionTester::PASSED) ? 0 : 1;
}
