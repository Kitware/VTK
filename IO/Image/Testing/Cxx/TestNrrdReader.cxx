// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkNrrdReader.h"

#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkImageMapper.h"
#include "vtkNew.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

int TestNrrdReader(int argc, char* argv[])
{
  char* filename1 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.nrrd");
  char* filename2 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.ascii.nhdr");
  char* filename3 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach_gzip.nrrd");
  if ((filename1 == nullptr) || (filename2 == nullptr) || (filename3 == nullptr))
  {
    cerr << "Could not get file names.";
    return 1;
  }

  vtkNew<vtkNrrdReader> reader1;
  if (!reader1->CanReadFile(filename1))
  {
    cerr << "Reader reports " << filename1 << " cannot be read.";
    return 1;
  }
  reader1->SetFileName(filename1);
  reader1->Update();

  vtkNew<vtkImageMapper> mapper1;
  mapper1->SetInputConnection(reader1->GetOutputPort());
  mapper1->SetColorWindow(256);
  mapper1->SetColorLevel(127.5);

  vtkNew<vtkActor2D> actor1;
  actor1->SetMapper(mapper1);

  vtkNew<vtkRenderer> renderer1;
  renderer1->AddActor(actor1);

  vtkNew<vtkNrrdReader> reader2;
  if (!reader2->CanReadFile(filename2))
  {
    cerr << "Reader reports " << filename2 << " cannot be read.";
    return 1;
  }
  reader2->SetFileName(filename2);
  reader2->Update();

  vtkNew<vtkImageMapper> mapper2;
  mapper2->SetInputConnection(reader2->GetOutputPort());
  mapper2->SetColorWindow(1.0);
  mapper2->SetColorLevel(0.5);

  vtkNew<vtkActor2D> actor2;
  actor2->SetMapper(mapper2);

  vtkNew<vtkRenderer> renderer2;
  renderer2->AddActor(actor2);

  vtkNew<vtkNrrdReader> reader3;
  if (!reader3->CanReadFile(filename3))
  {
    cerr << "Reader reports " << filename3 << " cannot be read.";
    return 1;
  }
  reader3->SetFileName(filename3);
  reader3->Update();

  vtkNew<vtkImageMapper> mapper3;
  mapper3->SetInputConnection(reader3->GetOutputPort());
  mapper3->SetColorWindow(256);
  mapper3->SetColorLevel(127.5);

  vtkNew<vtkActor2D> actor3;
  actor3->SetMapper(mapper3);

  vtkNew<vtkRenderer> renderer3;
  renderer3->AddActor(actor3);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 100);

  renderer1->SetViewport(0.0, 0.0, 0.333, 1.0);
  renderWindow->AddRenderer(renderer1);

  renderer2->SetViewport(0.333, 0.0, 0.666, 1.0);
  renderWindow->AddRenderer(renderer2);

  renderer3->SetViewport(0.666, 0.0, 1.0, 1.0);
  renderWindow->AddRenderer(renderer3);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindow->Render();
    interactor->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  delete[] filename1;
  delete[] filename2;
  delete[] filename3;

  return (retVal != vtkRegressionTester::PASSED);
}
