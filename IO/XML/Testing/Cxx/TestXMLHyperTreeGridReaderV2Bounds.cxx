// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGrid.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkTesting.h"
#include "vtkXMLHyperTreeGridReader.h"

#include <array>
#include <cstdlib>
#include <string>

int TestXMLHyperTreeGridReaderV2Bounds(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    vtkLog(ERROR, "Error: -D /path/to/data was not specified.");
    return EXIT_FAILURE;
  }

  std::string dataRoot = testHelper->GetDataRoot();
  std::string shell3DName = dataRoot + "/Data/HTG/shell_3d.htg"; // XML HTG v2 file

  vtkNew<vtkXMLHyperTreeGridReader> reader;
  reader->SetFileName(shell3DName.c_str());
  reader->Update();

  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());
  if (htg->GetNumberOfNonEmptyTrees() != 24)
  {
    vtkLog(ERROR, << "Expected " << 24 << " trees but got " << htg->GetNumberOfNonEmptyTrees());
    return EXIT_FAILURE;
  }

  double bounds[6];
  htg->GetBounds(bounds);
  std::array<double, 6> expectedBounds{ 0.5, 1.5, -1.0, 1.0, -1.0, 1.0 };
  for (int i = 0; i < 6; i++)
  {
    if (bounds[i] != expectedBounds[i])
    {
      vtkLog(ERROR, << "Expected bound " << i << " to be " << expectedBounds[i] << " but got "
                    << bounds[i]);
      return EXIT_FAILURE;
    }
  }

  // Make sure that changing coordinates BB  reduces the number of trees read from the file
  reader->SetCoordinatesBoundingBox(0, 1, 0, 1, 0, 1);
  reader->Update();
  if (htg->GetNumberOfNonEmptyTrees() != 4)
  {
    vtkLog(ERROR, << "Expected " << 4 << " trees but got " << htg->GetNumberOfNonEmptyTrees());
    return EXIT_FAILURE;
  }

  std::array<double, 6> expectedReducedBounds{ 0.5, 1.5, 0.0, 1.0, 0.0, 1.0 };
  htg->GetBounds(bounds);
  for (int i = 0; i < 6; i++)
  {
    if (bounds[i] != expectedReducedBounds[i])
    {
      vtkLog(ERROR, << "Expected reduced bound " << i << " to be " << expectedReducedBounds[i]
                    << " but got " << bounds[i]);
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
