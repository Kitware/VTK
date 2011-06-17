/*=========================================================================

  Program:   Visualization Toolkit
  Module:    EnzoDumpVariable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME EnzoDumpVariable.cxx -- {Enter documentation here!}
//
// .SECTION Description
//  TODO: Enter documentation here!

#include <iostream>
#include <cassert>
#include <cmath>

#include "vtkAMREnzoReader.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkUniformGrid.h"
#include "vtkAMRBox.h"
#include "AMRCommon.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"

int main( int argc, char **argv )
{
  if( argc != 4 )
    {
      std::cout << "Usage: EnzoDumpVariable <file> <var> <level>\n";
      std::cout.flush();
      return( -1 );
    }

  // STEP 0: Read AMR Data
  vtkAMREnzoReader *myReader = vtkAMREnzoReader::New();
  myReader->SetMaxLevel(  atoi( argv[3] ) );
  myReader->SetFileName( argv[1] );
  myReader->Update();

  myReader->SetCellArrayStatus( "Density", 1 );
  myReader->Update();

  vtkHierarchicalBoxDataSet *amrds = myReader->GetOutput();
  assert( "pre: AMR dataset is NULL" && (amrds != NULL) );

  // STEP 1: Loop and dump data
  unsigned int levelIdx = 0;
  for( ; levelIdx < amrds->GetNumberOfLevels(); ++levelIdx )
    {
      unsigned int dataIdx = 0;
      for( ; dataIdx < amrds->GetNumberOfDataSets( levelIdx ); ++dataIdx )
        {
          vtkUniformGrid *grd = amrds->GetDataSet( levelIdx, dataIdx );
          assert( "pre: grid should not be equal to NULL" && (grd != NULL) );

          vtkDataArray *da = grd->GetCellData()->GetArray( argv[2] );
          assert( "pre: data array is NULL" && (da != NULL) );

          std::cout << "BLOCK(" << levelIdx << ", " << dataIdx << "):\n";
          std::cout.flush();
          vtkIdType idx = 0;
          for( ; idx < da->GetNumberOfTuples(); ++idx  )
            {
              std::cout << "\t " << da->GetTuple1( idx ) << std::endl;
              std::cout.flush();
            } // END for all values in the array

        } // END for all data
    } // END for all levels
}
