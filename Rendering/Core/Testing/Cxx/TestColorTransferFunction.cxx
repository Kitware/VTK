/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestColorTransferFunctionStringArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkColorTransferFunction.h>
#include <vtkSmartPointer.h>

int TestColorTransferFunction(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();

  // Test for crash when getting table with no points
  ctf->RemoveAllPoints();

  // Range should be [0, 0]. Honest to goodness 0, not just very close to 0.
  double range[2];
  ctf->GetRange(range);

  if (range[0] != 0.0 || range[1] != 0.0)
  {
    std::cerr << "After RemoveAllPoints() is called, range should be [0, 0]. "
              << "It was [" << range[0] << ", " << range[1] << "].\n";
    return EXIT_FAILURE;
  }

  double table[256*3];
  ctf->GetTable(0.0, 1.0, 256, table);

  // Table should be all black.
  for (int i = 0; i < 3*256; ++i)
  {
    if (table[i] != 0.0)
    {
      std::cerr << "Table should have all zeros.\n";
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
