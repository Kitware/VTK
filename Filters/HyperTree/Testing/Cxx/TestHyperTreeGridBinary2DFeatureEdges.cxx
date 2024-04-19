// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Regression test for a binary 2D XY-oriented HTG containing masked cells.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkHyperTreeGridFeatureEdges.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLHyperTreeGridReader.h"

namespace
{
template <typename T1, typename T2>
bool testValue(T1 gotVal, T2 expectedVal, const char* valName)
{
  if (gotVal != expectedVal)
  {
    std::cerr << "Wrong " << valName << ". Expected " << expectedVal << ", got " << gotVal
              << std::endl;
    return false;
  }
  return true;
}
}

int TestHyperTreeGridBinary2DFeatureEdges(int argc, char* argv[])
{
  // HTG reader
  vtkNew<vtkXMLHyperTreeGridReader> reader;

  // Load data : binary 2D XY-oriented HTG containing masked cells
  char* fileNameC =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/binary_2D_XY_331_mask.htg");
  reader->SetFileName(fileNameC);
  delete[] fileNameC;

  // Geometry filter
  vtkNew<vtkHyperTreeGridFeatureEdges> featureEdgesFilter;
  featureEdgesFilter->SetInputConnection(reader->GetOutputPort());
  featureEdgesFilter->Update();

  vtkPolyData* geometry = featureEdgesFilter->GetPolyDataOutput();
  if (!geometry)
  {
    std::cerr << "Unable to retrieve htg geometry." << endl;
    return EXIT_FAILURE;
  }

  // Test number of points / cells (without MergePoints)
  if (!testValue(geometry->GetNumberOfPoints(), 104, "number of points"))
  {
    return EXIT_FAILURE;
  }
  if (!testValue(geometry->GetNumberOfCells(), 52, "number of cells"))
  {
    return EXIT_FAILURE;
  }

  featureEdgesFilter->SetMergePoints(true);
  featureEdgesFilter->Update();

  // Test number of points (with MergePoints)
  if (!testValue(geometry->GetNumberOfPoints(), 51, "number of points"))
  {
    return EXIT_FAILURE;
  }

  // Mapper
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(geometry);

  // Actor's properties
  vtkNew<vtkProperty> propety;
  propety->SetLineWidth(2.0);

  // Actors
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->SetProperty(propety);

  // Renderer and camera
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->ResetCamera();

  // Render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);

  // Interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Render and baseline test
  renWin->Render();
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
