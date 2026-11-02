// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFWriterDistributedTestUtilities.h"

#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArray.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

#include <cstdlib>

#define ASSERT(condition, message)                                                                 \
  do                                                                                               \
  {                                                                                                \
    if (!(condition))                                                                              \
    {                                                                                              \
      vtkLog(ERROR, << message);                                                                   \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

using namespace HDFWriterDistributedTestUtilities;

namespace
{
bool TestSource(int rank)
{
  for (const auto polyhedrons : { PolyhedronsType::NoPolyhedrons, PolyhedronsType::Polyhedrons })
  {
    Configuration config;
    config.Polyhedrons = polyhedrons;
    config.Temporal = TemporalType::Temporal;
    auto source = MakeConfigurationSource(config);
    for (int step = 0; step < NumberOfSteps; ++step)
    {
      source->UpdateTimeStep(step);
      auto* grid = vtkUnstructuredGrid::SafeDownCast(source->GetOutputDataObject(0));
      const bool polyhedron = polyhedrons == PolyhedronsType::Polyhedrons;
      const vtkIdType numberOfCells = step + 1;
      ASSERT(grid, "Expected an unstructured grid at step " << step);
      ASSERT(grid->GetNumberOfCells() == numberOfCells, "Unexpected cell count at step " << step);
      ASSERT(grid->GetNumberOfPoints() == numberOfCells * (polyhedron ? 8 : 4),
        "Unexpected point count at step " << step);
      ASSERT(grid->GetCellType(0) == (polyhedron ? VTK_POLYHEDRON : VTK_QUAD),
        "Unexpected cell type at step " << step);

      auto* pointValues = grid->GetPointData()->GetArray("PointValues");
      auto* cellValues = grid->GetCellData()->GetArray("CellValues");
      const double identity = 10000.0 * rank + step;
      ASSERT(pointValues, "Missing PointValues array at step " << step);
      ASSERT(pointValues->GetNumberOfTuples() == grid->GetNumberOfPoints(),
        "Unexpected PointValues tuple count at step " << step);
      ASSERT(pointValues->GetComponent(0, 0) == identity,
        "Unexpected PointValues value at step " << step);
      ASSERT(cellValues, "Missing CellValues array at step " << step);
      ASSERT(cellValues->GetNumberOfTuples() == numberOfCells,
        "Unexpected CellValues tuple count at step " << step);
      ASSERT(
        cellValues->GetComponent(numberOfCells - 1, 0) == identity + 0.25 * (numberOfCells - 1),
        "Unexpected CellValues value at step " << step);
    }
  }
  return true;
}
}

int TestHDFConfigurationSource(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMPIController::SetGlobalController(controller);

  const bool result = TestSource(controller->GetLocalProcessId());

  vtkMPIController::SetGlobalController(nullptr);
  controller->Finalize();
  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
