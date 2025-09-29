// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * This test checks reading a file with metadata-defined offset using the PDAL reader.
 */

#include "vtkNew.h"
#include "vtkPDALReader.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <vtksys/SystemTools.hxx>

namespace
{

bool FirstPoint(vtkPolyData* pd, std::array<double, 3>& p)
{
  if (!pd || !pd->GetPoints() || pd->GetPoints()->GetNumberOfPoints() == 0)
  {
    return false;
  }

  pd->GetPoints()->GetPoint(0, p.data());
  return true;
}

} // namespace

int TestPDALReaderWithOffset(int argc, char** argv)
{
  char* expanded = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/test_3.las");
  std::string file = expanded ? expanded : "";

  std::cerr << "[ReaderWithOffset] file: " << (file.empty() ? "<none>" : file) << "\n";
  if (file.empty() || !vtksys::SystemTools::FileExists(file))
  {
    delete[] expanded;
    std::cerr << "No valid LAS file found.\n";
    return EXIT_FAILURE;
  }

  // AutoOffset = false
  vtkNew<vtkPDALReader> r0;
  r0->SetFileName(file.c_str());
  r0->SetApplyOffset(false);
  r0->Update();
  std::array<double, 3> p0;
  if (!FirstPoint(r0->GetOutput(), p0))
  {
    delete[] expanded;
    std::cerr << "Failed to read first point (AutoOffset OFF).\n";
    return EXIT_FAILURE;
  }

  // AutoOffset = true
  vtkNew<vtkPDALReader> r1;
  r1->SetFileName(file.c_str());
  r1->SetApplyOffset(true);
  r1->Update();
  std::array<double, 3> p1;
  if (!FirstPoint(r1->GetOutput(), p1))
  {
    delete[] expanded;
    std::cerr << "Failed to read first point (AutoOffset ON).\n";
    return EXIT_FAILURE;
  }

  double dy = p1[1] - p0[1];
  std::cout << "y(OFF) = " << p0[1] << ", y(ON) = " << p1[1] << ", Δy = " << dy << std::endl;

  delete[] expanded;

  const double eps = 1e-6;
  return (std::abs(dy - 250.0) < eps) ? EXIT_SUCCESS : EXIT_FAILURE;
}
