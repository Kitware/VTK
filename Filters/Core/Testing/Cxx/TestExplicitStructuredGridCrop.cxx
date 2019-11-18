/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExplicitStructuredGridCrop.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This test read a unstructured grid and crop an explicit grid using
// vtkExplicitGrid

#include "vtkActor.h"
#include "vtkDataSetMapper.h"
#include "vtkExplicitStructuredGridCrop.h"
#include "vtkNew.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGridToExplicitStructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"

int TestExplicitStructuredGridCrop(int argc, char* argv[])
{
  vtkNew<vtkXMLUnstructuredGridReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/explicitStructuredGrid.vtu");
  reader->SetFileName(fname);
  delete[] fname;
  reader->Update();

  vtkNew<vtkUnstructuredGridToExplicitStructuredGrid> esgConvertor;
  esgConvertor->SetInputConnection(reader->GetOutputPort());
  esgConvertor->SetWholeExtent(0, 5, 0, 13, 0, 3);
  esgConvertor->SetInputArrayToProcess(0, 0, 0, 1, "BLOCK_I");
  esgConvertor->SetInputArrayToProcess(1, 0, 0, 1, "BLOCK_J");
  esgConvertor->SetInputArrayToProcess(2, 0, 0, 1, "BLOCK_K");
  esgConvertor->Update();

  vtkNew<vtkExplicitStructuredGridCrop> crop;
  crop->SetInputConnection(esgConvertor->GetOutputPort());
  crop->SetOutputWholeExtent(0, 5, 0, 6, 0, 3);
  crop->Update();

  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(crop->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
