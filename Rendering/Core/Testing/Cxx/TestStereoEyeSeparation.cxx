// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Tests left/right image offset when doing off-axis projection.  By placing
// one object behind the display surface, one directly on it, and one in front,
// this test ensures that image separation is zero at the screen and
// correctly swapped when object is behind, compared to in front, of the
// screen.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDiskSource.h"
#include "vtkMatrix4x4.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestStereoEyeSeparation(int argc, char* argv[])
{
  double bottomLeft[3] = { -4.0, -4.0, 0.0 };
  double bottomRight[3] = { 4.0, -4.0, 0.0 };
  double topRight[3] = { 4.0, 4.0, 0.0 };

  vtkNew<vtkDiskSource> disk[3];
  vtkNew<vtkPolyDataMapper> map[3];
  vtkNew<vtkActor> act[3];

  vtkNew<vtkRenderWindow> renwin;
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderer> ren;

  double diskCenters[9] = {
    0.0, 2.0, -4.0, // far disk is 4 units behind the screen
    0.0, 0.0, 0.0,  // middle disk is coincident with the screen
    0.0, -1.0, 4.0  // near disk is 4 units in front of the screen
  };

  for (int i = 0; i < 3; ++i)
  {
    disk[i]->SetCenter(diskCenters[i * 3 + 0], diskCenters[i * 3 + 1], diskCenters[i * 3 + 2]);
    disk[i]->SetInnerRadius(0.0);
    disk[i]->SetOuterRadius(0.5);
    disk[i]->SetNormal(0, 0, -1);
    disk[i]->SetCircumferentialResolution(100);

    map[i]->SetInputConnection(disk[i]->GetOutputPort());
    act[i]->SetMapper(map[i]);
    ren->AddActor(act[i]);
  }

  double eyePosition[3] = { 0.0, 0.0, 8.0 };

  vtkCamera* camera = ren->GetActiveCamera();
  camera->SetScreenBottomLeft(bottomLeft);
  camera->SetScreenBottomRight(bottomRight);
  camera->SetScreenTopRight(topRight);
  camera->SetUseOffAxisProjection(1);
  camera->SetEyePosition(eyePosition);
  camera->SetEyeSeparation(0.15);

  renwin->AddRenderer(ren);
  renwin->SetSize(400, 400);
  renwin->SetStereoCapableWindow(1);
  renwin->SetStereoTypeToRedBlue();
  renwin->SetStereoRender(1);

  iren->SetRenderWindow(renwin);
  renwin->Render();

  int retVal = vtkRegressionTestImage(renwin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  return (!retVal);
}
