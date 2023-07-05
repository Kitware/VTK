// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenFOAMReader.h"

#include "vtkCellData.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkGeometryFilter.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkUnstructuredGrid.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

int TestOpenFOAMReaderWeighByCellSize(int argc, char* argv[])
{
  // Read file name.
  char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/OpenFOAM/squareBend/squareBend.foam");

  // Read the file
  vtkNew<vtkOpenFOAMReader> reader;
  reader->SetFileName(filename);
  delete[] filename;

  reader->SetTimeValue(100);
  // These method names will be revised in the future
  reader->SetCreateCellToPoint(true);
  reader->SetSizeAverageCellToPoint(true);
  reader->SetDecomposePolyhedra(true);

  // Dont currently need anything selected (controlled above)
  reader->Update();

  vtkNew<vtkGeometryFilter> geometry;
  geometry->SetInputConnection(reader->GetOutputPort());
  geometry->Update();

  auto tree = vtkDataObjectTree::SafeDownCast(geometry->GetOutputDataObject(0));
  if (!tree)
  {
    std::cout << "Geometry filter did not output a composite tree data set" << std::endl;
    return EXIT_FAILURE;
  }
  for (auto dO : vtk::Range(tree,
         vtk::DataObjectTreeOptions::SkipEmptyNodes | vtk::DataObjectTreeOptions::VisitOnlyLeaves |
           vtk::DataObjectTreeOptions::TraverseSubTree))
  {
    auto ds = vtkDataSet::SafeDownCast(dO);
    ds->GetPointData()->SetScalars(ds->GetPointData()->GetArray("p"));
  }

  // Visualize
  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(geometry->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarRange(1e5, 2e5);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->SetBackground(0, 0, 0);

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return EXIT_SUCCESS;
}
