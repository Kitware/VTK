// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLUniformGridAMRReader.h"

#include <iostream>
#include <string>

// Each rank reads some of the blocks of an overlapping AMR, but the blanking of a block must be
// the same as in a serial read, even when its children are held by another rank. The AMR has one
// block on level 0 and two blocks on level 1, and it is read with 4 ranks, so that a rank can also
// have no block.
int TestParallelXMLAMRBlanking(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  char* dataRoot = vtkTestUtilities::GetDataRoot(argc, argv);
  if (!dataRoot)
  {
    std::cerr << "Error: -D /path/to/data was not specified." << std::endl;
    controller->Finalize();
    return EXIT_FAILURE;
  }
  const std::string fileName = std::string(dataRoot) + "/Data/AMR/amrug.vthb";
  delete[] dataRoot;

  const int myRank = controller->GetLocalProcessId();
  const int numberOfRanks = controller->GetNumberOfProcesses();

  vtkNew<vtkXMLUniformGridAMRReader> serialReader;
  serialReader->SetFileName(fileName.c_str());
  serialReader->SetMaximumLevelsToReadByDefault(0);
  serialReader->UpdatePiece(0, 1, 0);
  auto serialAMR = vtkOverlappingAMR::SafeDownCast(serialReader->GetOutputDataObject(0));

  vtkNew<vtkXMLUniformGridAMRReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->SetMaximumLevelsToReadByDefault(0);
  reader->UpdatePiece(myRank, numberOfRanks, 0);
  auto amr = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));

  bool success = serialAMR && amr && amr->GetNumberOfLevels() == serialAMR->GetNumberOfLevels();

  // The test checks nothing if the first level is not blanked by the second one in the serial read
  vtkIdType numberOfSerialBlankedCells = 0;
  for (unsigned int index = 0; success && index < serialAMR->GetNumberOfBlocks(0); ++index)
  {
    auto serialGhosts = vtkUnsignedCharArray::SafeDownCast(
      serialAMR->GetDataSetAsImageData(0, index)->GetCellData()->GetArray(
        vtkDataSetAttributes::GhostArrayName()));
    for (vtkIdType cell = 0; serialGhosts && cell < serialGhosts->GetNumberOfValues(); ++cell)
    {
      numberOfSerialBlankedCells += serialGhosts->GetValue(cell) != 0 ? 1 : 0;
    }
  }
  if (success && numberOfSerialBlankedCells == 0)
  {
    std::cerr << "The first level is not blanked in the serial read" << std::endl;
    success = false;
  }
  // The comparison includes the AMR boxes, the blocks that are present and their ghost arrays,
  // which hold the blanking, and skips the blocks that are on another rank
  if (success && !vtkTestUtilities::CompareDataObjects(amr, serialAMR))
  {
    std::cerr << "The distributed AMR differs from the serial one" << std::endl;
    success = false;
  }

  int numberOfLocalBlocks = 0;
  for (unsigned int level = 0; success && level < amr->GetNumberOfLevels(); ++level)
  {
    for (unsigned int index = 0; index < amr->GetNumberOfBlocks(level); ++index)
    {
      numberOfLocalBlocks += amr->GetDataSetAsImageData(level, index) ? 1 : 0;
    }
  }

  // Make sure the blocks are distributed, otherwise the test checks nothing
  int numberOfBlocks = 0;
  controller->AllReduce(&numberOfLocalBlocks, &numberOfBlocks, 1, vtkCommunicator::SUM_OP);
  if (success && numberOfBlocks != static_cast<int>(serialAMR->GetNumberOfBlocks()))
  {
    std::cerr << "Expected " << serialAMR->GetNumberOfBlocks()
              << " blocks over all ranks but found " << numberOfBlocks << std::endl;
    success = false;
  }

  int localSuccess = success ? 1 : 0;
  int globalSuccess = 0;
  controller->AllReduce(&localSuccess, &globalSuccess, 1, vtkCommunicator::MIN_OP);
  controller->Finalize();
  return globalSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
}
