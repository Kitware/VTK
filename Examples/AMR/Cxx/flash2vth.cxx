/*=========================================================================

  Program:   Visualization Toolkit
  Module:    flash2vth.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME flash2vth.cxx -- Converts an FLASH dataset into a *vth file.
//
// .SECTION Description
//  A simple utility that converts an FLASH AMR datasets into a vth,
//  hiearchical box AMR dataset.
//
// .SECTION See Also
// vtkXMLHierarchicalBoxDataWriter{Enter documentation here!}


#include <iostream>

#include "vtkAMRFlashReader.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "AMRCommon.h"

int main( int argc, char **argv )
{

  if( argc != 3 )
    {
      std::cout << "Usage: enzo2vth <file> <max-resolution> \n";
      std::cout.flush();
      return -1;
    }


  vtkAMRFlashReader *myReader = vtkAMRFlashReader::New();
  myReader->SetMaxLevel( atoi(argv[2]) );
  myReader->SetFileName( argv[1] );
  myReader->Update();

  vtkHierarchicalBoxDataSet *amrds = myReader->GetOutput();

  AMRCommon::WriteAMRData( amrds, "AMR" );

  myReader->Delete();
  return 0;
}
