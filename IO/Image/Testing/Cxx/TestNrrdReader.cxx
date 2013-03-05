/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNrrdReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkNrrdReader.h"

#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkImageMapper.h"
#include "vtkNew.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkStdString.h"
#include "vtkTestUtilities.h"

int TestNrrdReader(int argc, char *argv[])
{
  char *filename1 =
      vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.nrrd");
  char *filename2 =
      vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.ascii.nhdr");
  if ((filename1 == NULL) || (filename2 == NULL))
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
  actor1->SetMapper(mapper1.GetPointer());

  vtkNew<vtkRenderer> renderer1;
  renderer1->AddActor(actor1.GetPointer());

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
  actor2->SetMapper(mapper2.GetPointer());

  vtkNew<vtkRenderer> renderer2;
  renderer2->AddActor(actor2.GetPointer());

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(200, 100);

  renderer1->SetViewport(0.0, 0.0, 0.5, 1.0);
  renderWindow->AddRenderer(renderer1.GetPointer());

  renderer2->SetViewport(0.5, 0.0, 1.0, 1.0);
  renderWindow->AddRenderer(renderer2.GetPointer());

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow.GetPointer());

  int retVal = vtkRegressionTestImage(renderWindow.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    renderWindow->Render();
    interactor->Start();
    retVal = vtkRegressionTester::PASSED;
    }

  delete[] filename1;
  delete[] filename2;

  return (retVal != vtkRegressionTester::PASSED);

}
