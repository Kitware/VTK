// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStripper.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtkTriangleFilter.h"

int TestTriangleStrips(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.jpg");
  vtkNew<vtkJPEGReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkTexture> texture;
  texture->SetInputConnection(reader->GetOutputPort());
  texture->InterpolateOn();

  vtkNew<vtkPlaneSource> plane;
  vtkNew<vtkTriangleFilter> triangles;
  triangles->SetInputConnection(plane->GetOutputPort());
  vtkNew<vtkStripper> stripper;
  stripper->SetInputConnection(triangles->GetOutputPort());
  stripper->Update();

  vtkNew<vtkPolyData> removed;
  removed->DeepCopy(stripper->GetOutput());
  vtkNew<vtkPolyData> restored;
  restored->DeepCopy(stripper->GetOutput());
  vtkNew<vtkCellArray> strips;
  strips->DeepCopy(restored->GetStrips());

  vtkNew<vtkPolyDataMapper> removedMapper;
  removedMapper->SetInputData(removed);
  vtkNew<vtkActor> removedActor;
  removedActor->SetMapper(removedMapper);
  removedActor->SetPosition(-0.6, 0, 0);
  removedActor->GetProperty()->SetTexture("mytexture", texture);

  vtkNew<vtkPolyDataMapper> restoredMapper;
  restoredMapper->SetInputData(restored);
  vtkNew<vtkActor> restoredActor;
  restoredActor->SetMapper(restoredMapper);
  restoredActor->SetPosition(0.6, 0, 0);
  restoredActor->GetProperty()->SetTexture("mytexture", texture);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(removedActor);
  renderer->AddActor(restoredActor);
  renderer->SetBackground(0.5, 0.7, 0.7);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(600, 300);
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renWin);

  renderer->ResetCamera();
  renWin->Render();

  vtkNew<vtkCellArray> empty;
  removed->SetStrips(empty);
  restored->SetStrips(empty);
  renWin->Render();

  restored->SetStrips(strips);
  renWin->Render();

  const int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return !retVal;
}
