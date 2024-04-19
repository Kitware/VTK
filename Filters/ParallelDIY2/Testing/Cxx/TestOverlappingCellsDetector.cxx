// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkGenerateGlobalIds.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkOverlappingCellsDetector.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkXMLUnstructuredGridReader.h"

namespace
{
constexpr vtkIdType Collisions[72] = { 6, 0, 6, 0, 4, 4, 6, 0, 10, 7, // 0
  4, 0, 0, 7, 9, 0, 0, 5, 5, 0,                                       // 10
  0, 0, 5, 9, 0, 6, 0, 6, 4, 4,                                       // 20
  0, 6, 1, 0, 4, 8, 7, 7, 1, 7,                                       // 30
  5, 0, 0, 5, 7, 5, 0, 2, 0, 0,                                       // 40
  0, 0, 0, 6, 1, 4, 0, 1, 0, 0,                                       // 50
  4, 0, 0, 0, 0, 0, 2, 6, 0, 0,                                       // 60
  0, 0 };
}

static constexpr vtkIdType CollisionsWithTolerance[72] = { 4, 0, 4, 0, 1, 1, 4, 0, 4, 3, // 0
  1, 0, 0, 2, 5, 0, 0, 5, 5, 0,                                                          // 10
  0, 0, 5, 9, 0, 4, 0, 4, 1, 1,                                                          // 20
  0, 4, 1, 0, 1, 4, 3, 4, 1, 5,                                                          // 30
  5, 0, 0, 5, 7, 5, 0, 2, 0, 0,                                                          // 40
  0, 0, 0, 4, 1, 2, 0, 1, 0, 0,                                                          // 50
  2, 0, 0, 0, 0, 0, 2, 6, 0, 0,                                                          // 60
  0, 0 };

static constexpr vtkIdType CollisionsBlocks[144] = { 29, 13, 27, 12, 12, 13, 20, 7, 30, 22, // 0
  13, 7, 3, 21, 25, 4, 10, 19, 20, 12,                                                      // 10
  10, 8, 25, 29, 8, 18, 14, 23, 12, 14,                                                     // 20
  1, 15, 1, 5, 15, 26, 25, 20, 4, 28,                                                       // 30
  15, 1, 7, 17, 27, 24, 8, 14, 9, 12,                                                       // 40
  12, 13, 4, 20, 5, 15, 3, 8, 8, 3,                                                         // 50
  21, 5, 7, 10, 10, 5, 12, 23, 9, 10,                                                       // 60
  10, 12, 26, 10, 23, 14, 12, 11, 21, 3,                                                    // 70
  31, 21, 13, 3, 5, 19, 26, 4, 5, 22,                                                       // 80
  20, 10, 12, 10, 18, 27, 13, 31, 12, 21,                                                   // 90
  17, 10, 7, 20, 7, 1, 12, 28, 18, 22,                                                      // 100
  2, 24, 26, 8, 12, 15, 25, 16, 8, 12,                                                      // 110
  14, 11, 10, 10, 9, 18, 5, 20, 5, 4,                                                       // 120
  4, 11, 20, 7, 3, 7, 10, 10, 14, 22,                                                       // 130
  9, 8, 5, 10 };

int TestOverlappingCellsDetector(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> contr;
#else
  vtkNew<vtkDummyController> contr;
#endif
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  int retVal = EXIT_SUCCESS;
  int myrank = contr->GetLocalProcessId();

  const char* Tetname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/multiblock_overlapping_tetras/multiblock_overlapping_tetras_0_0.vtu");
  const char* MultiBlockTetname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/multiblock_overlapping_tetras.vtm");
  const char* Hexname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/linhex.vtu");
  const char* TetHexname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tet_hex.vtu");

  vtkNew<vtkGenerateGlobalIds> globalIds;

  vtkLogStartScope(TRACE, "Overlapping Tetras");
  if (myrank == 0)
  {
    vtkNew<vtkXMLUnstructuredGridReader> reader;
    reader->SetFileName(Tetname);
    globalIds->SetInputConnection(reader->GetOutputPort());
    globalIds->Update();
  }
  else
  {
    vtkNew<vtkUnstructuredGrid> ug;
    ug->Initialize();
    globalIds->SetInputDataObject(ug);
  }

  vtkNew<vtkRedistributeDataSetFilter> redistribute;
  redistribute->SetInputConnection(globalIds->GetOutputPort());

  vtkNew<vtkOverlappingCellsDetector> detector;
  detector->SetInputConnection(redistribute->GetOutputPort());
  detector->Update();

  {
    vtkDataSet* output = vtkDataSet::SafeDownCast(detector->GetOutput(0));
    vtkDataArray* data =
      output->GetCellData()->GetArray(detector->GetNumberOfOverlapsPerCellArrayName());
    vtkDataArray* ids = output->GetCellData()->GetArray("GlobalCellIds");

    auto valIt = vtk::DataArrayValueRange<1>(data).cbegin();
    auto idIt = vtk::DataArrayValueRange<1>(ids).cbegin();
    for (; valIt != vtk::DataArrayValueRange<1>(data).cend(); ++valIt, ++idIt)
    {
      if (Collisions[static_cast<vtkIdType>(*idIt)] != *valIt)
      {
        std::cerr << "Overlapping cells detector failed with an unstructured grid" << std::endl;
        retVal = EXIT_FAILURE;
        break;
      }
    }
  }

  vtkLogEndScope("Overlapping Tetras");
  vtkLogStartScope(TRACE, "Overlapping Tetras with tolerance");

  detector->SetTolerance(0.05);
  detector->Update();
  detector->SetTolerance(0.0);

  {
    vtkDataSet* output = vtkDataSet::SafeDownCast(detector->GetOutput(0));
    vtkDataArray* data =
      output->GetCellData()->GetArray(detector->GetNumberOfOverlapsPerCellArrayName());
    vtkDataArray* ids = output->GetCellData()->GetArray("GlobalCellIds");

    auto valIt = vtk::DataArrayValueRange<1>(data).cbegin();
    auto idIt = vtk::DataArrayValueRange<1>(ids).cbegin();
    for (; valIt != vtk::DataArrayValueRange<1>(data).cend(); ++valIt, ++idIt)
    {
      if (CollisionsWithTolerance[static_cast<vtkIdType>(*idIt)] != *valIt)
      {
        std::cerr << "Overlapping cells detector failed with an unstructured grid and tolerance"
                  << std::endl;
        retVal = EXIT_FAILURE;
        break;
      }
    }
  }

  vtkLogEndScope("Overlapping Tetras with tolerance");
  vtkLogStartScope(TRACE, "MultiBlock Overlapping Tetras");

  if (myrank == 0)
  {
    vtkNew<vtkXMLMultiBlockDataReader> reader;
    reader->SetFileName(MultiBlockTetname);
    globalIds->SetInputConnection(reader->GetOutputPort());
    globalIds->Update();
  }
  else
  {
    vtkNew<vtkMultiBlockDataSet> mbds;
    mbds->Initialize();
    globalIds->SetInputDataObject(mbds);
  }

  detector->SetInputConnection(globalIds->GetOutputPort());
  detector->Update();

  vtkMultiBlockDataSet* outputs = vtkMultiBlockDataSet::SafeDownCast(detector->GetOutput(0));
  auto iter = outputs->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (auto output = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()))
    {
      vtkDataArray* data =
        output->GetCellData()->GetArray(detector->GetNumberOfOverlapsPerCellArrayName());
      vtkDataArray* ids = output->GetCellData()->GetArray("GlobalCellIds");
      auto valIt = vtk::DataArrayValueRange<1>(data).cbegin();
      auto idIt = vtk::DataArrayValueRange<1>(ids).cbegin();
      for (; valIt != vtk::DataArrayValueRange<1>(data).cend(); ++valIt, ++idIt)
      {
        if (CollisionsBlocks[static_cast<vtkIdType>(*idIt)] != *valIt)
        {
          std::cerr << "Overlapping cells detector failed with a multi-block input" << std::endl;
          retVal = EXIT_FAILURE;
          break;
        }
      }
    }
  }
  iter->Delete();

  vtkLogEndScope("MultiBlock Overlapping Tetras");
  vtkLogStartScope(TRACE, "Overlapping Hexes");

  // Here we test a data set that used to produce an infinite loop
  // when computing the bounding sphere.
  // This dataset also checks that empty processes don't make the filter crash.
  if (myrank == 0)
  {
    vtkNew<vtkXMLUnstructuredGridReader> reader;
    reader->SetFileName(Hexname);
    detector->SetInputConnection(reader->GetOutputPort());
  }
  else
  {
    vtkNew<vtkUnstructuredGrid> ug;
    ug->Initialize();
    detector->SetInputDataObject(ug);
  }
  detector->Update();

  {
    vtkDataSet* output = vtkDataSet::SafeDownCast(detector->GetOutput(0));
    vtkDataArray* data =
      output->GetCellData()->GetArray(detector->GetNumberOfOverlapsPerCellArrayName());
    for (const auto val : vtk::DataArrayValueRange<1>(data))
    {
      if (val)
      {
        std::cerr << "Overlapping cells detector detected overlaps on a non-overlapping dataset"
                  << std::endl;
        retVal = EXIT_FAILURE;
        break;
      }
    }
  }
  vtkLogEndScope("Overlapping Hexes");

  vtkLogStartScope(TRACE, "Overlapping Tets and Hexes mixture");

  // We test data that has a mixture of tets and hexes
  if (myrank == 0)
  {
    vtkNew<vtkXMLUnstructuredGridReader> reader;
    reader->SetFileName(TetHexname);
    detector->SetInputConnection(reader->GetOutputPort());
  }
  else
  {
    vtkNew<vtkUnstructuredGrid> ug;
    ug->Initialize();
    detector->SetInputDataObject(ug);
  }
  detector->Update();

  {
    vtkDataSet* output = vtkDataSet::SafeDownCast(detector->GetOutput(0));
    vtkDataArray* data =
      output->GetCellData()->GetArray(detector->GetNumberOfOverlapsPerCellArrayName());
    for (const auto val : vtk::DataArrayValueRange<1>(data))
    {
      if (val)
      {
        std::cerr << "Overlapping cells detector detected overlaps on a non-overlapping dataset"
                  << std::endl;
        retVal = EXIT_FAILURE;
        break;
      }
    }
  }
  vtkLogEndScope("Overlapping Tets and Hexes mixture");

  delete[] TetHexname;
  delete[] Hexname;
  delete[] MultiBlockTetname;
  delete[] Tetname;

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return retVal;
}
