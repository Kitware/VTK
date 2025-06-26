// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkSimplePointsReader and vtkSimplePointsWriter
// .SECTION Description
//
#include "vtkAMREnzoReader.h"
#include "vtkCompositeDataReader.h"
#include "vtkCompositeDataWriter.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkOverlappingAMRMetaData.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUniformGrid.h"

namespace
{
vtkSmartPointer<vtkOverlappingAMR> CreateTestAMR(int argc, char* argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMR/Enzo/DD0010/moving7_0010.hierarchy");
  vtkNew<vtkAMREnzoReader> reader;
  reader->SetFileName(fname);
  delete[] fname;
  reader->SetMaxLevel(8);
  reader->SetCellArrayStatus("TotalEnergy", 1);
  reader->Update();
  vtkSmartPointer<vtkOverlappingAMR> ret =
    vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  return ret;
}
}

int TestAMRReadWrite(int argc, char* argv[])
{
  vtkSmartPointer<vtkOverlappingAMR> amr = CreateTestAMR(argc, argv);
  if (!amr->CheckValidity())
  {
    return EXIT_FAILURE;
  }

  vtkNew<vtkCompositeDataWriter> writer;
  writer->SetInputData(amr);
  writer->SetFileName("testamr");
  writer->Update();

  vtkNew<vtkCompositeDataReader> reader;
  reader->SetFileName("testamr");
  reader->Update();

  vtkSmartPointer<vtkOverlappingAMR> amr1 =
    vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  if (!amr1->CheckValidity())
  {
    return EXIT_FAILURE;
  }

  return (*amr1->GetOverlappingAMRMetaData() == *amr->GetOverlappingAMRMetaData()) ? EXIT_SUCCESS
                                                                                   : EXIT_FAILURE;
}
