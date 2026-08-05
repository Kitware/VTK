// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTexture.h"

#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"

/**
 * This test verifies that actor level materials work with the ANARI back-end
 */
int TestAnariSphere(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  // set up the environment
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0, 0, 0);
  renWin->AddRenderer(renderer);
  renWin->SetSize(700, 700);

  vtkAnariTestUtilities::SetParameterDefaults(renWin, useDebugDevice, "TestAnariSphere");

  // make some predictable data to test with
  // anything will do, but should have normals and textures coordinates
  // for materials to work with
  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(5);
  sphere->SetPhiResolution(100);
  sphere->SetThetaResolution(100);
  // measure it so we can automate positioning
  sphere->Update();
  double* bds = sphere->GetOutput()->GetBounds();
  double xo = bds[0];
  double xr = bds[1] - bds[0];
  double yo = bds[2];
  // double yr = bds[1]-bds[0];
  double zo = bds[4];
  double zr = bds[1] - bds[0];

  // now what we actually want to test.
  // draw the data at different places
  // varying the visual characteristics each time

  int i, j = 0;
  vtkProperty* prop;

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // plain old color
  i = 0;
  j = 0;
  {
    vtkNew<vtkActor> actor1;
    actor1->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);

    prop = actor1->GetProperty();
    prop->SetMaterialName("matte");
    prop->SetColor(1.0, 0.0, 0.0); // Red

    vtkNew<vtkPolyDataMapper> mapper1;
    mapper1->SetInputConnection(sphere->GetOutputPort());
    actor1->SetMapper(mapper1);

    renderer->AddActor(actor1);
  }

  // color mapping
  j++;
  {
    vtkNew<vtkActor> actor2;
    actor2->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);

    vtkNew<vtkPolyDataMapper> mapper2;
    vtkNew<vtkPolyData> copy1;
    copy1->ShallowCopy(sphere->GetOutput());
    mapper2->SetInputData(copy1);

    vtkNew<vtkDoubleArray> da1;
    da1->SetNumberOfComponents(0);
    da1->SetName("test_array");

    for (int c = 0; c < copy1->GetNumberOfPoints(); c++)
    {
      da1->InsertNextValue(c / (double)copy1->GetNumberOfPoints());
    }

    copy1->GetPointData()->SetScalars(da1);
    actor2->SetMapper(mapper2);
    renderer->AddActor(actor2);
  }

  j++;
  {
    vtkNew<vtkActor> actor3;
    actor3->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);

    vtkNew<vtkPolyDataMapper> mapper3;
    vtkNew<vtkPolyData> copy2;
    copy2->ShallowCopy(sphere->GetOutput());
    mapper3->SetInputData(copy2);

    vtkNew<vtkDoubleArray> da2;
    da2->SetNumberOfComponents(0);
    da2->SetName("test_array");

    for (int c = 0; c < copy2->GetNumberOfCells(); ++c)
    {
      da2->InsertNextValue(c / (double)copy2->GetNumberOfCells());
    }
    copy2->GetCellData()->SetScalars(da2);
    actor3->SetMapper(mapper3);
    renderer->AddActor(actor3);
  }

  // invalid material, should warn but draw matte material
  i = 1;
  j = 0;
  {
    vtkNew<vtkActor> actor4;
    actor4->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
    prop = actor4->GetProperty();
    prop->SetMaterialName("flubber");
    prop->SetColor(0.0, 0.0, 0.5); // Navy
    vtkNew<vtkPolyDataMapper> mapper4;
    mapper4->SetInputConnection(sphere->GetOutputPort());
    actor4->SetMapper(mapper4);
    renderer->AddActor(actor4);
  }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // matte
  j++;
  {
    vtkNew<vtkActor> actor5;
    actor5->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
    prop = actor5->GetProperty();
    prop->SetMaterialName("matte");
    prop->SetColor(0.0, 0.5, 0.0); // Green
    vtkNew<vtkPolyDataMapper> mapper5;
    mapper5->SetInputConnection(sphere->GetOutputPort());
    actor5->SetMapper(mapper5);
    renderer->AddActor(actor5);
  }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // transparent matte
  j++;
  {
    vtkNew<vtkActor> actor6;
    actor6->SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j);
    prop = actor6->GetProperty();
    prop->SetMaterialName("transparentMatte");
    prop->SetOpacity(0.5);
    prop->SetColor(0.5, 0.0, 0.5); // Purple
    vtkNew<vtkPolyDataMapper> mapper6;
    mapper6->SetInputConnection(sphere->GetOutputPort());
    actor6->SetMapper(mapper6);
    renderer->AddActor(actor6);
  }

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(30); // adjust to show more
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
