// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Reads a single exodus file in parallel.
 */
#include <vtkDataArray.h>
#include <vtkIOSSReader.h>
#include <vtkMPIController.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkTestUtilities.h>

#include "vtk_netcdf.h"
#if VTK_MODULE_USE_EXTERNAL_vtknetcdf
#if defined(NC_HAVE_META_H)
#include "netcdf_meta.h"
#endif
#endif

static std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

int TestIOSSExodusSingleFileParallel(int argc, char* argv[])
{
  vtkNew<vtkMPIController> contr;
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  vtkIOSSReader* reader = vtkIOSSReader::New();
  const auto fname = GetFileName(argc, argv, "Data/can.ex2");
  reader->SetFileName(fname.c_str());
  reader->SetController(contr);
  reader->Update();

  int retVal = EXIT_SUCCESS, globalRetVal = EXIT_SUCCESS;
  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));
#if NC_HAS_PARALLEL4 && NC_HAS_PNETCDF
  if (pdc->GetNumberOfCells() != 7152 / contr->GetNumberOfProcesses())
  {
    retVal = EXIT_FAILURE;
  }
#else
  if (contr->GetLocalProcessId() == 0 && pdc->GetNumberOfCells() != 7152)
  {
    retVal = EXIT_FAILURE;
  }
#endif
  contr->AllReduce(&retVal, &globalRetVal, 1, vtkCommunicator::MAX_OP);

  reader->SetController(nullptr);
  reader->Delete();
  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return globalRetVal;
}
