// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridContour.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"
#include "vtkXMLHyperTreeGridReader.h"
#include "vtkXMLPolyDataReader.h"

#include <iostream>

int TestHyperTreeGridBinary3DContourImplicitInteger(int argc, char* argv[])
{
  // Hyper tree grid containing integer cell data array
  vtkNew<vtkXMLHyperTreeGridReader> reader;
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/shell_3d.htg");
  reader->SetFileName(fileNameC);
  delete[] fileNameC;
  reader->Update();

  // Set scalars to contour with (unsigned char array)
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());
  htg->GetCellData()->SetScalars(htg->GetCellData()->GetArray("level"));

  // Contour
  vtkNew<vtkHyperTreeGridContour> contour;
  contour->SetInputConnection(reader->GetOutputPort());
  int nContours = 3;
  contour->SetNumberOfContours(nContours);
  // Explicitly set floating values
  contour->SetValue(0, 2.5);
  contour->SetValue(1, 3.5);
  contour->SetValue(2, 3.);

  // Use implicit arrays to store contouring values in the output contour
  contour->SetUseImplicitArrays(true);
  contour->Update();

  const std::string baselinePath =
    "/Data/vtkHDF/TestHyperTreeGridBinary3DContourImplicitInteger.vtkhdf";

  // Compare generated contour with baseline contour
  vtkPolyData* contourPd = contour->GetPolyDataOutput();

  if (!vtkTestUtilities::RegressionTest(argc, argv, contourPd, baselinePath))
  {
    std::cerr << "The generated contour does not match expected one." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
