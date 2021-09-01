/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOSSExodusRestarts.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Reads a exodus restart via a metafile
 */
#include <vtkCommunicator.h>
#include <vtkDataArray.h>
#include <vtkDataObject.h>
#include <vtkDataSet.h>
#include <vtkIOSSReader.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPointData.h>
#include <vtkTestUtilities.h>

#include <cmath>

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

static std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

int TestIOSSExodusRestarts(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkMPIController* contr = vtkMPIController::New();
#else
  vtkDummyController* contr = vtkDummyController::New();
#endif
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  const int myId = contr->GetLocalProcessId();
  const int numProcs = contr->GetNumberOfProcesses();

  vtkNew<vtkIOSSReader> reader;
  const auto fname = GetFileName(argc, argv, "Data/Exodus/ExRestarts/blow.ex-timeseries");
  reader->SetFileName(fname.c_str());
  reader->SetController(contr);
  reader->UpdateInformation();
  reader->UpdateTimeStep(9.0, myId, numProcs);

  int retVal = EXIT_SUCCESS;
  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));
  auto pd = pdc->GetPartitionedDataSet(0);
  auto ds = (pd && pd->GetNumberOfPartitions() > 0) ? pd->GetPartition(0) : nullptr;
  auto array = ds ? ds->GetPointData()->GetArray("thickness") : nullptr;
  double range[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  if (array)
  {
    array->GetRange(range, 0);
  }

  if (numProcs > 1)
  {
    double all_range[2];
    contr->AllReduce(range, all_range, 1, vtkCommunicator::MIN_OP);
    contr->AllReduce(range + 1, all_range + 1, 1, vtkCommunicator::MAX_OP);
    std::copy(all_range, all_range + 2, range);
  }

  if ((std::fabs(range[0] - 0.126328) > 0.0001) || (std::fabs(range[1] - 1.14768) > 0.0001))
  {
    vtkLogF(ERROR, "Failed since thickness range is not correct.");
    retVal = EXIT_FAILURE;
  }

  reader->SetController(nullptr);
  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  contr->Delete();
  return retVal;
}
