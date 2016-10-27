/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTRUCHASReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Input test to validate ability to read GE TRUCHAS files
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkTesting.h"
#include "vtkTestUtilities.h"
#include "vtkTRUCHASReader.h"
#include "vtkUnstructuredGrid.h"

#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

namespace {
  bool AE(double v1, double v2)
  {
    if (fabs(v2-v1) > 0.001)
    {
      cerr << v2 << "!=" << v1 << endl;
      return false;
    }
    return true;
  }
}

int TestTRUCHASReader(int argc, char *argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  char *fileName =
    vtkTestUtilities::ExpandDataFileName(argc,argv,
        "Data/TRUCHAS/moving_rod.h5");

  vtkNew<vtkTRUCHASReader> reader;
  reader->SetFileName(fileName);
  reader->UpdateInformation();
  reader->SetCellArrayStatus("dTdt", 0);
  int nb = reader->GetNumberOfBlockArrays();
  for (int b = 0; b < nb; b++)
  {
    cerr << "BLOCK ID " << b
         << " named " << reader->GetBlockArrayName(b) << endl;
  }
  reader->SetBlockArrayStatus("2", 0); //block nums start at 1
  reader->Update();
  vtkUnstructuredGrid *grid = vtkUnstructuredGrid::SafeDownCast
    (reader->GetOutput()->GetBlock(0));
  if (!grid)
  {
    cerr << "Could not open first block of known good file" << endl;
    return EXIT_FAILURE;
  }
  int rnb = 0; //we produce empty blocks when deselected
  for (int b = 0; b < nb; b++)
  {
  if (reader->GetOutput()->GetBlock(b))
    {
    rnb++;
    }
  }
  if ( (rnb != 1) ||
       (reader->GetOutput()->GetNumberOfBlocks() != 2) )
  {
    cerr << "Got unexpected number of blocks, found "
         << rnb << "/" << reader->GetOutput()->GetNumberOfBlocks()
         << " instead of "
         << 1 << "/" << 2
         << endl;
  }

  const int expectedNumArrays = 6;
  if (grid->GetCellData()->GetNumberOfArrays() != 6)
  {
    cerr << "Got unexpected number of arrays, found "
         << grid->GetCellData()->GetNumberOfArrays()
         << " instead of "
         << expectedNumArrays
         << endl;
    return EXIT_FAILURE;
  }

  const int expectedNumPoints = 2872;
  if (grid->GetNumberOfPoints()!=expectedNumPoints)
  {
    cerr << "Got unexpected number of points from file "
         << grid->GetNumberOfPoints()
         << " instead of "
         << expectedNumPoints
         << endl;
    return EXIT_FAILURE;
  }
  const int expectedNumCells = 1716;
  if (grid->GetNumberOfCells()!=expectedNumCells)
  {
    cerr << "Got unexpected number of cells from file "
         << grid->GetNumberOfCells()
         << " instead of "
         << expectedNumCells
         << endl;
    return EXIT_FAILURE;
  }
  vtkDoubleArray *da = vtkDoubleArray::SafeDownCast(grid->GetCellData()->GetArray("Grad_T"));
  double *ptr = da->GetTuple(42);
  if (!AE(*ptr,-135.675) || !AE(*(ptr+1),-62.7603) || !AE(*(ptr+2),2.01974))
  {
    cerr << "Got unexpected values from Grad_T array for cell 42 "
         << *ptr << "," << *(ptr+1) << "," <<  *(ptr+2)
         << " instead of "
         << -135.675 << "," << -62.7603 << "," <<  *(ptr+2)
         << endl;
    return EXIT_FAILURE;
  }

  reader->SetCellArrayStatus("dTdt", 1);
  reader->Update();
  if (grid->GetCellData()->GetNumberOfArrays() != expectedNumArrays+1)
  {
    cerr << "Got unexpected number of arrays, found "
         << grid->GetCellData()->GetNumberOfArrays()
         << " instead of "
         << expectedNumArrays+1
         << endl;
    return EXIT_FAILURE;
  }

  vtkInformation *inf = reader->GetExecutive()->GetOutputInformation(0);
  int numTimes = inf->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double tAlpha = inf->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 0);
  double tOmega = inf->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), numTimes-1);
  if (numTimes != 61 || !AE(tAlpha, 0.0) || !AE(tOmega, 301.932))
  {
    cerr << "Got unexpected times." << endl;
    cerr << numTimes << " not " <<  61 << " times ";
    cerr << tAlpha << " not " <<  0.0 << " first time";
    cerr << tOmega << " not " <<  301.932 << " last time";
    return EXIT_FAILURE;
  }
  const double expectedRanges[5][2] =
  {
    {0,0},
    {-0.0360963,0.123483},
    {-0.0231966,0.108961},
    {-0.0222647,0.111447},
    {-0.0204965,0.111295},
  };

  for (int i = 0; i < 5; i++)
  {
    double tNext = tAlpha + i*(tOmega-tAlpha)/5;
    reader->UpdateTimeStep(tNext);
    grid = vtkUnstructuredGrid::SafeDownCast
      (reader->GetOutput()->GetBlock(0));
    da = vtkDoubleArray::SafeDownCast(grid->GetCellData()->GetArray("dTdt"));
    double *mM = da->GetRange();
    if (!AE(mM[0], expectedRanges[i][0]) ||
        !AE(mM[1], expectedRanges[i][1]))
    {
      cerr << "Got unexpected ranges at time " << tNext << " "
           << mM[0] <<"," << mM[1] << " instead of "
           << expectedRanges[i][0] << "," << expectedRanges[i][1] << endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
