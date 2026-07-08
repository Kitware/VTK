// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridComputeSides.h"
#include "vtkCellGridMapper.h"
#include "vtkCellGridReader.h"
#include "vtkDataArray.h"
#include "vtkFiltersCellGrid.h"
#include "vtkLongArray.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRenderingCellGrid.h"
#include "vtkStringToken.h"
#include "vtkTestUtilities.h"

#include <iostream>

namespace
{

// Replace cell type's shape "connectivity" array with a VTK_LONG copy.
//
// NOTE: the injection must happen at the cell-attribute level (CellTypeInfo /
// ArraysByRole). The renderer accesses connectivity through those cached array
// pointers, so a swap at the attribute-group (vtkDataSetAttributes) level is
// ignored.
bool CoerceShapeConnectivityToLong(vtkCellGrid* grid)
{
  using namespace vtk::literals;

  vtkCellAttribute* shape = grid->GetShapeAttribute();
  if (!shape)
  {
    std::cerr << "No shape attribute on the cell grid.\n";
    return false;
  }

  bool injectedAny = false;
  for (const auto& cellType : grid->GetCellTypes())
  {
    const vtkStringToken cellTypeToken(cellType);
    vtkCellAttribute::CellTypeInfo info = shape->GetCellTypeInfo(cellTypeToken);

    auto it = info.ArraysByRole.find("connectivity"_token);
    if (it == info.ArraysByRole.end() || !it->second)
    {
      continue;
    }
    vtkDataArray* conn = vtkDataArray::SafeDownCast(it->second);
    if (!conn)
    {
      continue;
    }

    // VTK_LONG by construction
    vtkNew<vtkLongArray> longConn;

    // copy name, components and values
    longConn->DeepCopy(conn);

    // swap in the VTK_LONG array
    it->second = longConn;

    // save the modified role map
    shape->SetCellTypeInfo(cellTypeToken, info);
    injectedAny = true;
  }
  return injectedAny;
}

} // anonymous namespace

int TestCellGridRenderingLongConnectivity(int argc, char* argv[])
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
  vtkRenderingCellGrid::RegisterCellsAndResponders();

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgHexahedra.dg");
  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(fname);
  delete[] fname;
  reader->Update();

  vtkCellGrid* grid = vtkCellGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!grid)
  {
    std::cerr << "Failed to read cell grid.\n";
    return EXIT_FAILURE;
  }

  // Mutate the connectivity into a 64-bit `long` (VTK_LONG).
  if (!CoerceShapeConnectivityToLong(grid))
  {
    std::cerr << "Failed to inject a VTK_LONG connectivity array.\n";
    return EXIT_FAILURE;
  }

  vtkNew<vtkCellGridComputeSides> sides;
  sides->SetInputDataObject(grid);
  sides->PreserveRenderableInputsOff();
  sides->OmitSidesForRenderableInputsOff();

  vtkNew<vtkCellGridMapper> mapper;
  mapper->SetInputConnection(sides->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
  mapper->SetArrayName("scalar1");

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.2, 0.3, 0.4);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  renderWindow->SetSize(300, 300);

  renderer->GetActiveCamera()->Azimuth(20);
  renderer->GetActiveCamera()->Elevation(30);
  renderer->ResetCamera();

  // Test passes as long as this Render doesn't trigger an abort (stemming
  // from the 8 byte connectivity array elements).
  renderWindow->Render();

  return EXIT_SUCCESS;
}
