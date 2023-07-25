// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLUnstructuredGridReader.h"

int TestExtractSurfaceNonLinearSubdivision(int argc, char* argv[])
{
  // Basic visualisation.
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0, 0, 0);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->SetSize(300, 300);

  vtkNew<vtkXMLUnstructuredGridReader> reader;
  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/quadraticTetra01.vtu");
  reader->SetFileName(filename);
  delete[] filename;
  filename = nullptr;

  vtkNew<vtkDataSetSurfaceFilter> extract_surface;
  extract_surface->SetInputConnection(reader->GetOutputPort());
  extract_surface->SetNonlinearSubdivisionLevel(4);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(extract_surface->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SelectColorArray("scalars");
  mapper->SetScalarModeToUsePointFieldData();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  ren->AddActor(actor);
  ren->ResetCamera();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
