// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkSimplePointsReader and vtkSimplePointsWriter
// .SECTION Description
//
#include "vtkAMREnzoReader.h"
#include "vtkCompositeDataWriter.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUniformGrid.h"

int TestAMRReadWrite(int argc, char* argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMR/Enzo/DD0010/moving7_0010.hierarchy");

  vtkNew<vtkAMREnzoReader> reader;
  reader->SetFileName(fname);
  reader->SetMaxLevel(8);
  reader->SetCellArrayStatus("TotalEnergy", 1);
  reader->Update();

  vtkSmartPointer<vtkOverlappingAMR> amr;
  amr = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  amr->Audit();

  vtkNew<vtkCompositeDataWriter> writer;
  writer->SetFileName();
  writer->Write();

  return EXIT_SUCCESS;
}
