// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkImageData.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkPolyData.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkSetGet.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

#include <iostream>

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

// Each AMR block is read by one rank only, with the same AMR boxes and the same blanking as a
// serial read
bool TestOverlappingAMR(vtkMPIController* controller, const std::string& dataRoot)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  std::string filename = dataRoot + "/Data/vtkHDF/amr_gaussian_pulse.vtkhdf";

  vtkNew<vtkHDFReader> serialReader;
  serialReader->SetFileName(filename.c_str());
  serialReader->UpdatePiece(0, 1, 0);
  auto serialAMR = vtkOverlappingAMR::SafeDownCast(serialReader->GetOutputDataObject(0));

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filename.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);
  auto amr = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));

  // The comparison includes the AMR boxes, the blocks that are present and their ghost arrays,
  // which hold the blanking, and skips the blocks that are on another rank
  if (!serialAMR || !amr || !vtkTestUtilities::CompareDataObjects(amr, serialAMR))
  {
    vtkErrorWithObjectMacro(nullptr, << "The distributed AMR differs from the serial one");
    return false;
  }

  // Blocks are dealt out to the ranks in turn, so each block is on exactly one rank
  for (unsigned int level = 0; level < amr->GetNumberOfLevels(); ++level)
  {
    for (unsigned int index = 0; index < amr->GetNumberOfBlocks(level); ++index)
    {
      const bool isMine = amr->GetAbsoluteBlockIndex(level, index) % nbRanks == myRank;
      if ((amr->GetDataSetAsImageData(level, index) != nullptr) != isMine)
      {
        vtkErrorWithObjectMacro(
          nullptr, << "Block " << level << "," << index << " is on the wrong rank");
        return false;
      }
    }
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
  res &= ::TestOverlappingAMR(controller, dataRoot);
  controller->Finalize();
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
