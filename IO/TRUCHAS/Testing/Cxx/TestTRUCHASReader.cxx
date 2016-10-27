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
#include "vtkPointData.h"
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
        "Data/TRUCHAS/viscoplastic-ring.h5");

  vtkNew<vtkTRUCHASReader> reader;
  reader->SetFileName(fileName);
  reader->UpdateInformation();
  int nb = reader->GetNumberOfBlockArrays();
  cerr << nb << " BLOCKS" << endl;
  for (int b = 0; b < nb; b++)
  {
    cerr << "BLOCK ID " << b
         << " named " << reader->GetBlockArrayName(b) << endl;
  }
  reader->SetBlockArrayStatus("1", 0); //block nums start at 1
  reader->SetBlockArrayStatus("2", 1); //block nums start at 1
  reader->SetBlockArrayStatus("3", 0); //block nums start at 1

  int nca = reader->GetNumberOfCellArrays();
  cerr << nca << " CELL ARRAYS" << endl;
  for (int a = 0; a < nca; a++)
  {
    cerr << "ARRAY " << a
         << " named " << reader->GetCellArrayName(a) << endl;
  }
  cerr << "IGNORE VOF" << endl;
  reader->SetCellArrayStatus("VOF", 0);

  int npa = reader->GetNumberOfPointArrays();
  cerr << npa << " POINT ARRAYS" << endl;
  for (int a = 0; a < npa; a++)
  {
    cerr << "ARRAY " << a
         << " named " << reader->GetPointArrayName(a) << endl;
  }
  cerr << "IGNORE Displacement" << endl;
  reader->SetPointArrayStatus("Displacement", 0);
  reader->Update();

  vtkUnstructuredGrid *grid = vtkUnstructuredGrid::SafeDownCast
    (reader->GetOutput()->GetBlock(1));
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
       (reader->GetOutput()->GetNumberOfBlocks() != 3) )
  {
    cerr << "Got unexpected number of blocks, found "
         << rnb << "/" << reader->GetOutput()->GetNumberOfBlocks()
         << " instead of "
         << 1 << "/" << 3
         << endl;
  }


  cerr << "---- CELL ARRAYS ----" << endl;
  const int expectedNumCArrays = nca-1;
  if (nca > 0 &&
      grid->GetCellData()->GetNumberOfArrays() != expectedNumCArrays)
  {
    cerr << "Got unexpected number of cell arrays, found "
         << grid->GetCellData()->GetNumberOfArrays()
         << " instead of "
         << expectedNumCArrays
         << endl;
    return EXIT_FAILURE;
  }
  for (int a = 0; a < grid->GetCellData()->GetNumberOfArrays(); a++)
    {
    vtkDataArray *da = grid->GetCellData()->GetArray(a);
    cerr << da->GetName() << endl;
    }

  cerr << "---- POINT ARRAYS ----" << endl;
  const int expectedNumPArrays = npa-1;
  if (npa > 0 &&
      grid->GetPointData()->GetNumberOfArrays() != expectedNumPArrays)
  {
    cerr << "Got unexpected number of point arrays, found "
         << grid->GetPointData()->GetNumberOfArrays()
         << " instead of "
         << expectedNumPArrays
         << endl;
    return EXIT_FAILURE;
  }
  for (int a = 0; a < grid->GetPointData()->GetNumberOfArrays(); a++)
    {
    vtkDataArray *da = grid->GetPointData()->GetArray(a);
    cerr << da->GetName() << endl;
    }

  const int expectedNumPoints = 496;
  if (grid->GetNumberOfPoints()!=expectedNumPoints)
  {
    cerr << "Got unexpected number of points from file "
         << grid->GetNumberOfPoints()
         << " instead of "
         << expectedNumPoints
         << endl;
    return EXIT_FAILURE;
  }
  const int expectedNumCells = 180;
  if (grid->GetNumberOfCells()!=expectedNumCells)
  {
    cerr << "Got unexpected number of cells from file "
         << grid->GetNumberOfCells()
         << " instead of "
         << expectedNumCells
         << endl;
    return EXIT_FAILURE;
  }

  vtkDoubleArray *da = vtkDoubleArray::SafeDownCast
    (grid->GetCellData()->GetArray("Grad_T"));
  if (!da)
    {
    cerr << "Couldn't get " << "Grad_T" << " array" << endl;
    return EXIT_FAILURE;
    }

  double *ptr = da->GetTuple(42);
  const double eVals[3] = {
    -10.4436, -4.32586, -10.4913
  };

  if (!AE(*ptr,eVals[0]) || !AE(*(ptr+1),eVals[1]) || !AE(*(ptr+2),eVals[2]))
  {
    cerr << "Got unexpected values from Grad_T array for cell 42 "
         << *ptr << "," << *(ptr+1) << "," <<  *(ptr+2)
         << " instead of "
         << eVals[0] << "," << eVals[1] << "," << eVals[2]
         << endl;
    return EXIT_FAILURE;
  }
  reader->SetCellArrayStatus("VOF", 1);
  reader->Update();
  if (grid->GetCellData()->GetNumberOfArrays() != expectedNumCArrays+1)
  {
    cerr << "Got unexpected number of cell arrays, found "
         << grid->GetCellData()->GetNumberOfArrays()
         << " instead of "
         << expectedNumCArrays+1
         << endl;
    return EXIT_FAILURE;
  }

  vtkInformation *inf = reader->GetExecutive()->GetOutputInformation(0);
  int numTimes = inf->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  cerr << "FOUND " << numTimes << " timesteps " << endl;
  double tAlpha = inf->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 0);
  double tOmega = inf->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), numTimes-1);
  const int expectedNumTimes = 2;
  const double expectedMinT = 0.0;
  const double expectedMaxT = 0.5;
  if (numTimes != 2 || !AE(tAlpha, expectedMinT) || !AE(tOmega, expectedMaxT))
  {
    cerr << "Got unexpected times." << endl;
    cerr << numTimes << " not " <<  expectedNumTimes << " times ";
    cerr << tAlpha << " not " <<  expectedMinT << " first time";
    cerr << tOmega << " not " <<  expectedMaxT << " last time";
    return EXIT_FAILURE;
  }
  const int divs = 3;
  const double expectedRanges[divs][2] =
  {
    {0,0}, //before
    {0,0}, //after first
    {-1.99025,-0.85729}, //after second
  };

  for (int i = 0; i < divs; i++)
  {
    double tNext = tAlpha-0.1 + i*(tOmega-tAlpha)*2.5/divs;
    reader->UpdateTimeStep(tNext);
    grid = vtkUnstructuredGrid::SafeDownCast
      (reader->GetOutput()->GetBlock(1));
    da = vtkDoubleArray::SafeDownCast(grid->GetCellData()->GetArray("dTdt"));
    double *mM = da->GetRange();
    cerr << "ts " << i << ":" << tNext
         << " got " << mM[0] << "," << mM[1] << endl;
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
