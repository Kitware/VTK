// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// Test the method ResetCameraScreenSpace
//
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCylinderSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

// int main( int argc, char* argv[] )
int TestResetCameraScreenSpace(int argc, char* argv[])
{
  vtkCylinderSource* c = vtkCylinderSource::New();
  c->SetHeight(8.0);

  vtkPolyDataMapper* m = vtkPolyDataMapper::New();
  m->SetInputConnection(c->GetOutputPort());

  vtkActor* a = vtkActor::New();
  a->SetMapper(m);
  a->RotateZ(90);
  a->RotateX(80);

  vtkRenderer* ren1 = vtkRenderer::New();
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  ren1->AddActor(a);
  ren1->SetBackground(0.1, 0.2, 0.4);

  renWin->SetSize(200, 300);

  ren1->GetActiveCamera()->SetUseHorizontalViewAngle(true);
  ren1->ResetCameraScreenSpace();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  c->Delete();
  m->Delete();
  a->Delete();
  ren1->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
