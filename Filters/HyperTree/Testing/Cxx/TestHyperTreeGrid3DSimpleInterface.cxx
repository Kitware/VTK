// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Regression test for a single-cell 3D HTG containing a defined interface.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLHyperTreeGridReader.h"

int TestHyperTreeGrid3DSimpleInterface(int argc, char* argv[])
{
  // HTG reader
  vtkNew<vtkXMLHyperTreeGridReader> reader;

  // Data containing both `InterfaceNormalsName` and `InterfaceInterceptsName` attributes
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/single_cell_3d.htg");
  reader->SetFileName(fileNameC);
  delete[] fileNameC;

  // Geometry filter
  vtkNew<vtkHyperTreeGridGeometry> geometryFilter;
  geometryFilter->SetInputConnection(reader->GetOutputPort());
  geometryFilter->Update();

  // Set active scalars for testing
  vtkPolyData* geometry = geometryFilter->GetPolyDataOutput();
  if (!geometry)
  {
    vtkLog(ERROR, "Unable to retrieve htg geometry.");
    return EXIT_FAILURE;
  }

  // Mapper
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(geometry);

  // Actors
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Renderer and camera
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->GetActiveCamera()->Azimuth(90);
  renderer->ResetCamera();

  // Render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);

  // Interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Render and test
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
