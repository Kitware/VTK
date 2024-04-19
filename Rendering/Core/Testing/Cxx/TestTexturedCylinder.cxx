// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**Description
 * Test for texturing over the cylinder source with and without capsular caps.
 */

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCylinderSource.h"
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkTexture.h"

int TestTexturedCylinder(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.jpg");

  vtkNew<vtkJPEGReader> reader;
  reader->SetFileName(fname);
  reader->Update();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(501, 200); // Intentional NPOT size

  vtkNew<vtkCylinderSource> capsule;
  capsule->CappingOn();
  capsule->CapsuleCapOn();
  capsule->SetResolution(12);
  capsule->SetRadius(5);
  capsule->SetHeight(10);
  capsule->LatLongTessellationOn();

  vtkNew<vtkCylinderSource> cylinder;
  cylinder->SetResolution(12);
  cylinder->SetRadius(5);
  cylinder->SetHeight(10);
  cylinder->SetCapping(true);

  vtkNew<vtkRenderer> ren[2];
  vtkNew<vtkTexture> texture[2];
  vtkNew<vtkPolyDataMapper> mapper[2];
  vtkNew<vtkActor> actor[2];

  int numVp = 2;
  for (int i = 0; i < numVp; ++i)
  {
    ren[i]->SetViewport(static_cast<double>(i) / numVp, 0, static_cast<double>(i + 1) / numVp, 1);
    ren[i]->SetBackground(0.3 * i + 0.3, 0.3 * i + 0.3, 0.3 * i + 0.3);
    renWin->AddRenderer(ren[i]);

    texture[i]->SetInputConnection(reader->GetOutputPort());
    texture[i]->SetBorderColor(0.5, 0.5, 0.5, 0.5);
    texture[i]->InterpolateOn();
    texture[i]->SetWrap(vtkTexture::Repeat);

    if (i < 1)
    {
      mapper[i]->SetInputConnection(capsule->GetOutputPort());
    }
    else
    {
      mapper[i]->SetInputConnection(cylinder->GetOutputPort());
    }
    actor[i]->SetMapper(mapper[i]);
    actor[i]->SetTexture(texture[i]);
    ren[i]->AddActor(actor[i]);
    if (i < 1)
    {
      ren[1]->GetActiveCamera()->SetPosition(0, 0, 50);
      ren[i]->GetActiveCamera()->SetFocalPoint(0, 0, 0);
      ren[i]->GetActiveCamera()->SetViewUp(0.6, -0.8, 0);
      ren[i]->ResetCamera();
      ren[i]->GetActiveCamera()->Zoom(1.3);
    }
    else
    {
      ren[i]->SetActiveCamera(ren[0]->GetActiveCamera());
    }
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  delete[] fname;
  return !retVal;
}
