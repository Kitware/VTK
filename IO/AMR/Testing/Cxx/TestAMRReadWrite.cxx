/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAMRReadWrite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkSimplePointsReader and vtkSimplePointsWriter
// .SECTION Description
//
#include "vtkSmartPointer.h"
#include "vtkSetGet.h"
#include "vtkOverlappingAMR.h"
#include "vtkUniformGrid.h"
#include "vtkNew.h"
#include "vtkAMREnzoReader.h"
#include "vtkTestUtilities.h"
#include "vtkCompositeDataWriter.h"

int TestAMRReadWrite( int argc, char *argv[] )
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv,"Data/AMR/Enzo/DD0010/moving7_0010.hierarchy");

  vtkNew<vtkAMREnzoReader> reader;
  reader->SetFileName(fname);
  reader->SetMaxLevel(8);
  reader->SetCellArrayStatus( "TotalEnergy",1);
  reader->Update();

  vtkSmartPointer<vtkOverlappingAMR> amr;
  amr = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  amr->Audit();


  vtkNew<vtkCompositeDataWriter> writer;
  writer->SetFileName();
  writer->Write();


  return EXIT_SUCCESS;
}
