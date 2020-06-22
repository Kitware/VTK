/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOverlappingCellsDetector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkGenerateGlobalIds.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkOverlappingCellsDetector.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"

namespace
{
static constexpr vtkIdType Collisions[72] = { 6, 0, 6, 0, 4, 4, 6, 0, 10, 7, // 0
  4, 0, 0, 7, 9, 0, 0, 5, 5, 0,                                              // 10
  0, 0, 5, 9, 0, 6, 0, 6, 4, 4,                                              // 20
  0, 6, 1, 0, 4, 8, 7, 7, 1, 7,                                              // 30
  5, 0, 0, 5, 7, 5, 0, 2, 0, 0,                                              // 40
  0, 0, 0, 6, 1, 4, 0, 1, 0, 0,                                              // 50
  4, 0, 0, 0, 0, 0, 2, 6, 0, 0,                                              // 60
  0, 0 };
}

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

  const char* name =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/overlapping_tetras.vtu");

  vtkNew<vtkGenerateGlobalIds> globalIds;

  if (myrank == 0)
  {
    vtkNew<vtkXMLUnstructuredGridReader> reader;
    reader->SetFileName(name);
    globalIds->SetInputConnection(reader->GetOutputPort());
    globalIds->Update();
  }
  else
  {
    vtkNew<vtkUnstructuredGrid> ug;
    ug->Initialize();
    globalIds->SetInputDataObject(ug);
  }

  delete[] name;

  vtkNew<vtkRedistributeDataSetFilter> redistribute;
  redistribute->SetInputConnection(globalIds->GetOutputPort());

  vtkNew<vtkOverlappingCellsDetector> detector;
  detector->SetInputConnection(redistribute->GetOutputPort());
  detector->Update();

  vtkDataSet* output = vtkDataSet::SafeDownCast(detector->GetOutput(0));
  vtkDataArray* data =
    output->GetCellData()->GetArray(detector->GetNumberOfCollisionsPerCellArrayName());
  vtkDataArray* ids = output->GetCellData()->GetArray("GlobalCellIds");

  auto valIt = vtk::DataArrayValueRange(data).cbegin();
  auto idIt = vtk::DataArrayValueRange(ids).cbegin();
  for (; valIt != vtk::DataArrayValueRange(data).cend(); ++valIt, ++idIt)
  {
    if (Collisions[static_cast<vtkIdType>(*idIt)] != *valIt)
    {
      retVal = EXIT_FAILURE;
      break;
    }
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return retVal;
}
