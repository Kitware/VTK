/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AMRHomogenization.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME AMRHomogenization.cxx -- Utility that generates homogenized AMR dataset
//
// .SECTION Description
//  A simple utility to demonstrate & test the vtkAMRHomogenization filter.

#include "AMRCommon.h"
#include "vtkAMRBox.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkXMLHierarchicalBoxDataWriter.h"
#include "vtkAMRHomogenizationFilter.h"

int main( int argc, char **argv )
{
  // STEP 0: Read in AMR dataset
  vtkHierarchicalBoxDataSet *amrData=
   AMRCommon::ReadAMRData( std::string(argv[1] ) );
  AMRCommon::WriteAMRData( amrData, std::string("INPUTAMR") );
  amrData->GenerateVisibilityArrays();
  assert( "AMR dataset is empty!"  && (amrData->GetNumberOfLevels() > 0) );


  vtkAMRHomogenizationFilter *homogenizer = vtkAMRHomogenizationFilter::New();
  homogenizer->SetInput( amrData );
  homogenizer->Update();
  vtkMultiBlockDataSet *output = homogenizer->GetOutput();

  std::cout << " Write homogenized grid...";
  std::cout.flush();
  AMRCommon::WriteMultiBlockData( output, "HOMOGENIZED" );
  std::cout << "[DONE]\n";
  std::cout.flush();
  return 0;
}
