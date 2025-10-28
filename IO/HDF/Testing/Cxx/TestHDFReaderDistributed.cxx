// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkSetGet.h"
#include "vtkTesting.h"

namespace
{

bool TestSinglePieceMultiblock(vtkMPIController* controller, const std::string& dataRoot)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  vtkNew<vtkHDFReader> reader;
  std::string filename = dataRoot + "/Data/vtkHDF/sphere_onepiece.vtkhdf";

  reader->SetFileName(filename.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);
  auto pd = vtkPolyData::SafeDownCast(
    vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0))->GetBlock(0));

  vtkIdType expectedNumCells = myRank == 0 ? 96 : 0;
  if (pd->GetNumberOfCells() != expectedNumCells)
  {
    vtkErrorWithObjectMacro(
      nullptr, << "Expected " << expectedNumCells << " cells but found " << pd->GetNumberOfCells());
    return false;
  }

  return true;
}
}

int TestHDFReaderDistributed(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified." << std::endl;
    return EXIT_FAILURE;
  }
  std::string dataRoot = testHelper->GetDataRoot();

  bool res = ::TestSinglePieceMultiblock(controller, dataRoot);
  controller->Finalize();
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
