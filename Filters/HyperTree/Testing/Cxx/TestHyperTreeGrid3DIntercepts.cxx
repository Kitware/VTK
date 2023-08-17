// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests the validity of "vtkInterfaceIntercepts", defining witch part of a cell
// to keep when the given cell contains an interface.

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

int TestHyperTreeGrid3DIntercepts(int argc, char* argv[])
{
  // HTG reader
  vtkNew<vtkXMLHyperTreeGridReader> reader;

  // Test data
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/three_cells_3d.htg");
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

  const std::string colorArray = "vtkInterfaceIntercepts";
  vtkDataArray* vectors =
    vtkDataArray::SafeDownCast(geometry->GetCellData()->GetAbstractArray(colorArray.c_str()));
  if (!vectors)
  {
    vtkLog(ERROR, "Unable to retrieve the " << colorArray << " array.");
    return EXIT_FAILURE;
  }

  // Assign a color value for each possible intercepts value
  vtkNew<vtkLookupTable> lut;
  lut->SetNumberOfTableValues(4);
  lut->SetTableValue(0, 0.23, 0.30, 0.75); // -1
  lut->SetTableValue(1, 0., 0., 0.);       // 0
  lut->SetTableValue(2, 0.87, 0.87, 0.87); // 1
  lut->SetTableValue(3, 0.70, 0.02, 0.15); // 2
  lut->SetVectorModeToComponent();
  lut->SetVectorComponent(2);
  lut->Build();

  // Mapper
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(geometry);
  mapper->SetLookupTable(lut);
  mapper->SetScalarRange(vectors->GetFiniteRange(2));
  mapper->SelectColorArray(colorArray.c_str());
  mapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
  mapper->ScalarVisibilityOn();
  mapper->SetColorModeToMapScalars();

  // Actors
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Renderer and camera
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->GetActiveCamera()->Azimuth(0);
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
