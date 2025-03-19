// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test covers rendering of translucent geometry along with anti-aliasing using MSAA
 */

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"

int TestOpacityMSAA(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkConeSource> c1;
  c1->SetResolution(1);
  c1->SetCenter(-0.5, 0, 0);
  c1->SetRadius(1.3);
  vtkNew<vtkPolyDataMapper> m1;
  m1->SetInputConnection(c1->GetOutputPort());
  vtkNew<vtkActor> a1;
  a1->SetMapper(m1);

  vtkNew<vtkConeSource> c2;
  c2->SetResolution(1);
  c2->SetCenter(0.5, 0, 0);
  c2->SetRadius(1.3);
  c2->SetDirection(-1, 0, 0);
  vtkNew<vtkPolyDataMapper> m2;
  m2->SetInputConnection(c2->GetOutputPort());
  vtkNew<vtkActor> a2;
  a2->SetMapper(m2);
  a2->GetProperty()->SetOpacity(0.5);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(8); // enable multisampling
  renWin->SetSize(301, 300);  // Intentional NPOT size

  vtkNew<vtkRenderer> ren;
  ren->AddActor(a1);
  ren->AddActor(a2);
  renWin->AddRenderer(ren);
  ren->ResetCamera();
  ren->GetActiveCamera()->Roll(4);
  ren->SetUseOIT(false); // disable OIT pass

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
