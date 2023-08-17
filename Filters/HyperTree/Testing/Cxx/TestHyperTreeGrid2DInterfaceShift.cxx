// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Regression test for 2D HTG containing masked cells and an interface.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkLogger.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLHyperTreeGridReader.h"

int TestHyperTreeGrid2DInterfaceShift(int argc, char* argv[])
{
  // HTG reader
  vtkNew<vtkXMLHyperTreeGridReader> reader;

  // Test data
  char* fileNameC =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/donut_XZ_shift_2d.htg");
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
  vtkDataArray* scalars =
    vtkDataArray::SafeDownCast(geometry->GetCellData()->GetAbstractArray("level"));
  if (!scalars)
  {
    vtkLog(ERROR, "Unable to retrieve \"level\" array.");
    return EXIT_FAILURE;
  }
  geometry->GetCellData()->SetScalars(scalars);

  // "Cool to Warm" lookup table
  vtkNew<vtkLookupTable> lut;
  lut->SetNumberOfTableValues(3);
  lut->SetTableValue(0, 0.23, 0.30, 0.75);
  lut->SetTableValue(1, 0.87, 0.87, 0.87);
  lut->SetTableValue(2, 0.70, 0.02, 0.15);

  // Mapper
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(geometry);
  mapper->SetLookupTable(lut);
  mapper->SetScalarRange(scalars->GetRange());

  // Force scalar mapping since "level" is an unsigned char array
  // so directly interpreted as RGB values by default
  mapper->SetColorModeToMapScalars();

  // Actors
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Renderer and camera
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->GetActiveCamera()->Roll(90);
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
