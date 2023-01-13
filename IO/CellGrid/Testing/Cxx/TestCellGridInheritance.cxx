// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGrid.h"
#include "vtkCellGridResponders.h"
#include "vtkCellMetadata.h"
#include "vtkDGTet.h"
#include "vtkDGTri.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIOCellGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkVector.h"

using namespace vtk::literals;

namespace
{

bool InheritanceHierarchyExists()
{
  vtkNew<vtkCellGrid> grid;
  bool ok = true;

  // Iterate over every registered cell type and ensure it overrides the
  // InheritanceHierarchy() method on its leaf node.
  for (const auto& cellTypeName : vtkCellMetadata::CellTypes())
  {
    auto cellType = vtkCellMetadata::NewInstance(cellTypeName.Data(), grid);
    if (!cellType)
    {
      std::cerr << "ERROR! Could not create cell of type " << cellTypeName.Data() << "\n";
      ok = false;
      continue;
    }
    auto classNames = cellType->InheritanceHierarchy();
    std::cout << "  " << cellTypeName.Data() << " hierarchy:\n    ";
    bool didFindSelf = false;
    bool once = true;
    for (const auto& className : classNames)
    {
      if (className == "vtkObject"_token)
      {
        break;
      }
      std::cout << (once ? " " : " → ") << className.Data();
      once = false;
      if (className == vtkStringToken(cellType->GetClassName()))
      {
        didFindSelf = true;
      }
    }
    std::cout << "\n";
    if (!didFindSelf)
    {
      std::cerr << "    ERROR! Could not find " << cellType->GetClassName() << "\n";
      ok = false;
    }
  }
  return ok;
}

} // anonymous namespace

int TestCellGridInheritance(int argc, char* argv[])
{
  (void)argc;
  (void)argv;
  vtkIOCellGrid::RegisterCellsAndResponders();
  if (!InheritanceHierarchyExists())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
