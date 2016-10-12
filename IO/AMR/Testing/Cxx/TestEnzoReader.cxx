/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestEnzoReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <iostream>
#include <string>

#include "vtkAMREnzoReader.h"
#include "vtkSetGet.h"
#include "vtkTestUtilities.h"
#include "vtkOverlappingAMR.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMRDataIterator.h"
namespace EnzoReaderTest {

//------------------------------------------------------------------------------
template<class T>
int CheckValue( const std::string &name, T actualValue, T expectedValue )
{
    if( actualValue != expectedValue )
    {
      std::cerr << "ERROR: " << name << " value mismatch! ";
      std::cerr << "Expected: " << expectedValue << " Actual: " << actualValue;
      std::cerr << std::endl;
      return 1;
    }
    return 0;
}

} // END namespace

int ComputeMaxNonEmptyLevel(vtkOverlappingAMR* amr)
{
  vtkUniformGridAMRDataIterator* iter = vtkUniformGridAMRDataIterator::SafeDownCast(amr->NewIterator());
  iter->SetSkipEmptyNodes(true);
  int maxLevel(-1);
  for(iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    int level = iter->GetCurrentLevel();
    if(level>maxLevel)
    {
      maxLevel = level;
    }
  }
  iter->Delete();
  return maxLevel+1;
}

int ComputeNumberOfVisibileCells(vtkOverlappingAMR* amr)
{
  int numVisibleCells(0);
  vtkCompositeDataIterator* iter = amr->NewIterator();
  iter->SkipEmptyNodesOn();
  for(iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast(iter->GetCurrentDataObject());
    vtkIdType num = grid->GetNumberOfCells();
    for(vtkIdType i=0; i<num; i++)
    {
      if(grid->IsCellVisible(i))
      {
        numVisibleCells++;
      }
    }
  }
  iter->Delete();
  return numVisibleCells;
}


int TestEnzoReader( int argc, char *argv[] )
{
  int rc = 0;
  int NumBlocksPerLevel[] = { 1,3,1,1,1,1,1,1 };
  int numVisibleCells[] = {4096, 6406, 13406, 20406, 23990, 25502, 26377, 27077};
  vtkAMREnzoReader *myEnzoReader = vtkAMREnzoReader::New();
  char *fileName =
    vtkTestUtilities::ExpandDataFileName(argc,argv,
        "Data/AMR/Enzo/DD0010/moving7_0010.hierarchy");
  std::cout << "Filename: " << fileName << std::endl;
  std::cout.flush();

  vtkOverlappingAMR *amr = NULL;
  myEnzoReader->SetFileName( fileName );
  for(int level = 0; level < myEnzoReader->GetNumberOfLevels(); ++level )
  {
    myEnzoReader->SetMaxLevel( level );
    myEnzoReader->Update();
    rc+=EnzoReaderTest::CheckValue("LEVEL",myEnzoReader->GetNumberOfLevels(),8);
    rc+=EnzoReaderTest::CheckValue("BLOCKS",myEnzoReader->GetNumberOfBlocks(),10);

    amr = myEnzoReader->GetOutput();
    amr->Audit();
    if( amr != NULL )
    {
      rc+=EnzoReaderTest::CheckValue(
        "OUTPUT LEVELS",static_cast<int>(ComputeMaxNonEmptyLevel(amr)),level+1);
      rc+=EnzoReaderTest::CheckValue(
          "NUMBER OF BLOCKS AT LEVEL",
          static_cast<int>(amr->GetNumberOfDataSets(level)),
          NumBlocksPerLevel[level]
          );
      rc+= EnzoReaderTest::CheckValue("Number of Visible cells ",ComputeNumberOfVisibileCells(amr), numVisibleCells[level]);
    }
    else
    {
      std::cerr << "ERROR: output AMR dataset is NULL!";
      return 1;
    }
  } // END for all levels

  myEnzoReader->Delete();
  delete [] fileName;
  return( rc );
}
