/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFlashReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <iostream>
#include <string>

#include "vtkAMRFlashReader.h"
#include "vtkSetGet.h"
#include "vtkTestUtilities.h"
#include "vtkOverlappingAMR.h"
#include "vtkUniformGridAMRDataIterator.h"

namespace FlashReaderTest {

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

} // END namespace

int TestFlashReader( int argc, char *argv[] )
{
  int rc = 0;
  int NumBlocksPerLevel[] = {1,8,64,512,3456,15344,78208};

  vtkAMRFlashReader *flashReader = vtkAMRFlashReader::New();

  char *fileName =
    vtkTestUtilities::ExpandDataFileName(argc,argv,
        "Data/AMR/Flash/smooth/smooth.flash");
  std::cout << "Filename: " << fileName << std::endl;
  std::cout.flush();

  vtkOverlappingAMR *amr = NULL;
  flashReader->SetFileName( fileName );
  for(int level = 0; level < flashReader->GetNumberOfLevels(); ++level )
  {
    flashReader->SetMaxLevel( level );
    flashReader->Update();
    rc+=FlashReaderTest::CheckValue("LEVEL",flashReader->GetNumberOfLevels(),7);
    rc+=FlashReaderTest::CheckValue("BLOCKS",flashReader->GetNumberOfBlocks(),97593);

    amr = flashReader->GetOutput();
    if( amr != NULL )
    {
      rc+=FlashReaderTest::CheckValue(
        "OUTPUT LEVELS",static_cast<int>(FlashReaderTest::ComputeMaxNonEmptyLevel(amr)),level+1);
      rc+=FlashReaderTest::CheckValue(
         "NUMBER OF BLOCKS AT LEVEL",
         static_cast<int>(amr->GetNumberOfDataSets(level)),
         NumBlocksPerLevel[level]
         );
    }
    else
    {
      std::cerr << "ERROR: output AMR dataset is NULL!";
      return 1;
    }
  } // END for all levels
  flashReader->Delete();
  delete [] fileName;
  return( rc );
}
