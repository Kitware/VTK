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
#include <vtkStringFormatter.h>
#include <vtkTestUtilities.h>
#include <vtksys/SystemTools.hxx>

static std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

int RunTest(int argc, char* argv[], vtkMPIController* contr, const std::string& datapath)
{
  const int numProcs = contr->GetNumberOfProcesses();

  const std::string filename = datapath + vtk::to_string(numProcs);
  const std::string filepath = GetFileName(argc, argv, filename);
  vtksys::SystemTools::PutEnv("CATALYST_READER_TIME_STEP=0");

  vtksys::SystemTools::PutEnv("CATALYST_DATA_DUMP_DIRECTORY=" + filepath);
  vtkNew<vtkIOSSReader> reader;
  reader->SetFileName("catalyst.bin");
  reader->SetCatalystConduitChannelName("channel_z");
  reader->Update();

  int retVal = EXIT_SUCCESS, globalRetVal = EXIT_SUCCESS;
  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));

  if (pdc->GetNumberOfCells() != 1 || pdc->GetNumberOfPartitionedDataSets() != 1)
  {
    retVal = EXIT_FAILURE;
  }

  contr->AllReduce(&retVal, &globalRetVal, 1, vtkCommunicator::MAX_OP);
  return globalRetVal;
}

int TestIOSSCatalystOneCell(int argc, char* argv[])
{
  vtkNew<vtkMPIController> contr;
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  int retVal = EXIT_SUCCESS;
  int rcu = RunTest(argc, argv, contr, "Data/Iocatalyst_one_cell_unstructured_MPI_");
  int rcs = RunTest(argc, argv, contr, "Data/Iocatalyst_one_cell_structured_MPI_");

  if (rcu == EXIT_FAILURE || rcs == EXIT_FAILURE)
  {
    retVal = EXIT_FAILURE;
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return retVal;
}
