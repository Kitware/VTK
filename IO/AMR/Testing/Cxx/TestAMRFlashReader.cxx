// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <iostream>
#include <string>

#include "vtkAMRFlashReader.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkOverlappingAMR.h"
#include "vtkSetGet.h"
#include "vtkTestUtilities.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMRDataIterator.h"
namespace FlashReaderTest
{

//------------------------------------------------------------------------------
template <class T>
int CheckValue(const std::string& name, T actualValue, T expectedValue)
{
  if (actualValue != expectedValue)
  {
    std::cerr << "ERROR: " << name << " value mismatch! ";
    std::cerr << "Expected: " << expectedValue << " Actual: " << actualValue;
    std::cerr << std::endl;
    return 1;
  }
  return 0;
}

} // END namespace

static int ComputeMaxNonEmptyLevel(vtkOverlappingAMR* amr)
{
  vtkUniformGridAMRDataIterator* iter =
    vtkUniformGridAMRDataIterator::SafeDownCast(amr->NewIterator());
  iter->SetSkipEmptyNodes(true);
  int maxLevel(-1);
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    int level = iter->GetCurrentLevel();
    if (level > maxLevel)
    {
      maxLevel = level;
    }
  }
  iter->Delete();
  return maxLevel + 1;
}

static void ComputeNumberOfCells(
  vtkOverlappingAMR* amr, int level, int& numCells, int& numVisibleCells)
{
  numCells = 0;
  numVisibleCells = 0;
  vtkUniformGridAMRDataIterator* iter =
    vtkUniformGridAMRDataIterator::SafeDownCast(amr->NewIterator());
  iter->SkipEmptyNodesOn();
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast(iter->GetCurrentDataObject());
    vtkIdType num = grid->GetNumberOfCells();
    if (level == (int)iter->GetCurrentLevel())
    {
      for (vtkIdType i = 0; i < num; i++)
      {
        if (grid->IsCellVisible(i))
        {
          numVisibleCells++;
        }
      }
      numCells += (int)num;
    }
  }
  iter->Delete();
}

int TestAMRFlashReader(int argc, char* argv[])
{
  int rc = 0;
  int NumBlocksPerLevel[] = { 27, 8 };
  int numCells[] = { 13824, 4096 };
  int numVisibleCells[] = { 13312, 4096 };
  vtkAMRFlashReader* myFlashReader = vtkAMRFlashReader::New();
  char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMR/Flash/SpitzerTest_hdf5_chk_0000");
  std::cout << "Filename: " << fileName << std::endl;
  std::cout.flush();

  vtkOverlappingAMR* amr = nullptr;
  myFlashReader->SetFileName(fileName);
  if (!myFlashReader || myFlashReader->GetNumberOfLevels() == 0)
  { // makeshift test for if the file was really loaded..
    std::cerr << "ERROR: input AMR dataset is invalid!";
    return 1;
  }
  for (int level = 0; level < myFlashReader->GetNumberOfLevels(); ++level)
  {
    myFlashReader->SetMaxLevel(level);
    myFlashReader->Update();
    rc += FlashReaderTest::CheckValue("LEVEL", myFlashReader->GetNumberOfLevels(), 2);
    rc += FlashReaderTest::CheckValue("BLOCKS", myFlashReader->GetNumberOfBlocks(), 35);

    amr = myFlashReader->GetOutput();
    amr->Audit();
    if (amr != nullptr)
    {
      rc += FlashReaderTest::CheckValue(
        "OUTPUT LEVELS", static_cast<int>(ComputeMaxNonEmptyLevel(amr)), 2);
      rc += FlashReaderTest::CheckValue("NUMBER OF BLOCKS AT LEVEL",
        static_cast<int>(amr->GetNumberOfDataSets(level)), NumBlocksPerLevel[level]);
      int nc = 0, nvc = 0;
      ComputeNumberOfCells(amr, level, nc, nvc);
      rc += FlashReaderTest::CheckValue("Number of cells ", nc, numCells[level]);
      rc += FlashReaderTest::CheckValue("Number of Visible cells ", nvc, numVisibleCells[level]);
    }
    else
    {
      std::cerr << "ERROR: output AMR dataset is nullptr!";
      return 1;
    }
  } // END for all levels

  myFlashReader->Delete();
  delete[] fileName;
  return (rc);
}
