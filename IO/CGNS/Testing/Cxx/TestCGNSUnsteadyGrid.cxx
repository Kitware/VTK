// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Jakub Benda, CFD support, Czech Republic
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCGNSReader.h"
#include "vtkCell.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <cmath>
#include <iostream>

#define vtk_assert(x)                                                                              \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      std::cerr << "On line " << __LINE__ << " ERROR: Condition FAILED!! : " << #x << std::endl;   \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

int TestOutput(vtkMultiBlockDataSet* mb, double size)
{
  constexpr double eps = 1e-5;
  unsigned int nBlocks = mb->GetNumberOfBlocks();
  std::cout << "nBlocks = " << nBlocks << std::endl;
  vtk_assert(nBlocks > 0);
  for (unsigned int i = 0; i < nBlocks; ++i)
  {
    std::cout << "Block #" << i << std::endl;
    vtkMultiBlockDataSet* mb2 = vtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(i));
    for (unsigned int j = 0; j < mb2->GetNumberOfBlocks(); ++j)
    {
      std::cout << " - Sub-block #" << j << std::endl;
      vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(mb2->GetBlock(j));
      int nc = ug->GetNumberOfCells();
      vtk_assert(nc == 1);
      for (vtkIdType k = 0; k < ug->GetNumberOfCells(); ++k)
      {
        std::cout << "    - cell #" << k << std::endl;
        vtkCell* cell = ug->GetCell(k);
        vtkPoints* points = cell->GetPoints();
        vtk_assert(cell->GetCellType() == VTK_HEXAHEDRON);
        for (vtkIdType l = 0; l < cell->GetNumberOfPoints(); ++l)
        {
          double x[3];
          points->GetPoint(cell->GetPointId(l), x);
          std::cout << "       - point #" << l << ": [ " << x[0] << ", " << x[1] << ", " << x[2]
                    << " ]" << std::endl;
          vtk_assert(std::fabs(std::fabs(x[0]) - size) < eps);
          vtk_assert(std::fabs(std::fabs(x[1]) - size) < eps);
          vtk_assert(std::fabs(std::fabs(x[2]) - size) < eps);
        }
      }
    }
  }
  return 0;
}

int TestCGNSUnsteadyGrid(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Example_grids.cgns");
  std::string grids = fname ? fname : "";
  delete[] fname;

  std::cout << "Opening " << grids << std::endl;
  vtkNew<vtkCGNSReader> gridsReader;
  gridsReader->SetFileName(grids.c_str());
  gridsReader->Update();

  vtkMultiBlockDataSet* mb = gridsReader->GetOutput();

  if (0 != TestOutput(mb, 1.0))
  {
    return EXIT_FAILURE;
  }

  gridsReader->GetOutputInformation(0)->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 1.0);
  gridsReader->Update();

  mb = gridsReader->GetOutput();

  if (0 != TestOutput(mb, 2.0))
  {
    return EXIT_FAILURE;
  }

  std::cout << __FILE__ << " tests passed." << std::endl;
  return EXIT_SUCCESS;
}
