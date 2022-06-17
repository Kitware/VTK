/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkLengthDistribution.h"

#include "vtkCellData.h"
#include "vtkDebugLeaks.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

namespace
{
} // anonymous namespace

//------------------------------------------------------------------------------
int TestLengthDistribution(int argc, char* argv[])
{
  int result = 0;
  vtkNew<vtkUnstructuredGridReader> mr;
  vtkNew<vtkLengthDistribution> ld;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/uGridEx.vtk");

  mr->SetFileName(fname);
  mr->Update();

  ld->SetInputConnection(mr->GetOutputPort());
  ld->Update();

  auto* cdf = ld->GetOutput();
  cdf->Dump(10);
  if (cdf->GetNumberOfRows() != 12)
  {
    std::cerr << "Wrong number of rows (" << cdf->GetNumberOfRows() << ")\n";
    ++result;
  }

  // The median length for this dataset is always 1.0:
  if (ld->GetLengthQuantile(0.5) != 1.0)
  {
    std::cerr << "Unexpected median length (" << ld->GetLengthQuantile(0.5) << ").\n";
    ++result;
  }

  bool didCatch = false;
  try
  {
    ld->GetLengthQuantile(-1.0); // force an exception.
  }
  catch (std::invalid_argument& e)
  {
    didCatch = true;
    std::cout << "Caught expected exception: \"" << e.what() << "\"\n";
  }
  if (!didCatch)
  {
    std::cerr << "Failed to throw expected exception for quantile < 0.\n";
    ++result;
  }

  didCatch = false;
  try
  {
    ld->GetLengthQuantile(2.0); // force an exception.
  }
  catch (std::invalid_argument& e)
  {
    didCatch = true;
    std::cout << "Caught expected exception: \"" << e.what() << "\"\n";
  }
  if (!didCatch)
  {
    std::cerr << "Failed to throw expected exception for quantile > 1.\n";
    ++result;
  }

  ld->SortSampleOff();
  ld->Update();
  cdf = ld->GetOutput();
  cdf->Dump(10);

  didCatch = false;
  try
  {
    ld->GetLengthQuantile(0.5); // force an exception.
  }
  catch (std::logic_error& e)
  {
    didCatch = true;
    std::cout << "Caught expected exception: \"" << e.what() << "\"\n";
  }
  if (!didCatch)
  {
    std::cerr << "Failed to throw expected exception unsorted lengths.\n";
    ++result;
  }

  delete[] fname;

  return result;
}
