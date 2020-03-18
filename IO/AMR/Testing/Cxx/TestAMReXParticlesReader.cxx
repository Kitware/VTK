/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAMReXParticlesReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMReXParticlesReader.h"
#include "vtkDataArraySelection.h"
#include "vtkIdTypeArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"

#define ensure(x, msg)                                                                             \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << "FAILED: " << msg << endl;                                                             \
    return EXIT_FAILURE;                                                                           \
  }

int Validate(vtkMultiBlockDataSet* mb)
{
  ensure(mb != nullptr, "expecting vtkMultiBlockDataSet.");
  ensure(mb->GetNumberOfBlocks() == 1, "expecting num-blocks == num-levels == 1");

  auto mp = vtkMultiPieceDataSet::SafeDownCast(mb->GetBlock(0));
  ensure(mp != nullptr, "expecting level is maintained in a vtkMultiPieceDataSet.");
  ensure(mp->GetNumberOfPieces() == 8, "expecting 8 datasets in level 0");
  for (int cc = 0; cc < 8; ++cc)
  {
    auto pd = vtkPolyData::SafeDownCast(mp->GetPiece(cc));
    ensure(pd != nullptr, "expecting polydata for index " << cc);
    ensure(pd->GetNumberOfPoints() > 0, "expecting non-null points.");
    ensure(pd->GetPointData()->GetArray("density") != nullptr, "missing density");
  }

  return EXIT_SUCCESS;
}

int TestAMReXParticlesReader(int argc, char* argv[])
{
  // Test 3D
  {
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMReX/MFIX-Exa/plt00000");
    vtkNew<vtkAMReXParticlesReader> reader;
    reader->SetPlotFileName(fname);
    delete[] fname;

    reader->SetParticleType("particles");
    reader->GetPointDataArraySelection()->DisableArray("proc");
    reader->UpdateInformation();
    ensure(reader->GetPointDataArraySelection()->ArrayIsEnabled("proc") == 0,
      "`proc` should be disabled.");
    reader->Update();
    if (Validate(reader->GetOutput()) == EXIT_FAILURE)
    {
      return EXIT_FAILURE;
    }
  }

  // Test 2D
  {
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMReX/Sample2D/plt00100");
    vtkNew<vtkAMReXParticlesReader> reader;
    reader->SetPlotFileName(fname);
    delete[] fname;

    reader->SetParticleType("Tracer");
    reader->UpdateInformation();
    reader->Update();

    double bds[6];
    reader->GetOutput()->GetBounds(bds);
    ensure(bds[4] == bds[5], "expecting 2D dataset");
  }

  return EXIT_SUCCESS;
}
