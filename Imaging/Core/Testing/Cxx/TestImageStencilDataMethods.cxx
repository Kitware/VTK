/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageStencilDataMethods.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test the IsInside method of vtkImageStencilData.

#include "vtkImageStencilData.h"
#include "vtkSmartPointer.h"
#include "vtkTesting.h"

//----------------------------------------------------------------------------
int TestImageStencilDataMethods(int argc, char *argv[])
{
  vtkSmartPointer<vtkTesting> testing =
    vtkSmartPointer<vtkTesting>::New();
  for (int cc = 1; cc < argc; cc ++ )
  {
    testing->AddArgument(argv[cc]);
  }

  // Test the IsInside method
  vtkSmartPointer<vtkImageStencilData> stencil3 =
    vtkSmartPointer<vtkImageStencilData>::New();
  stencil3->SetExtent(0, 11, 0, 0, 0, 0);
  stencil3->AllocateExtents();
  stencil3->InsertNextExtent(4, 7, 0, 0);
  stencil3->InsertNextExtent(9, 9, 0, 0);
  int expectedOut[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  int expectedIn[12] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0 };
  for (int idX = 0; idX < 12; idX++)
  {
    for (int idY = -1; idY <= 1; idY++)
    {
      for (int idZ = -1; idZ <= 1; idZ++)
      {
        int *expected = ((idY == 0 && idZ == 0) ? expectedIn : expectedOut);
        if (stencil3->IsInside(idX, idY, idZ) != expected[idX])
        {
          cerr << "IsInside(" << idX << ", " << idY << ", " << idZ << ") failed\n";
          return EXIT_FAILURE;
        }
      }
    }
  }

  return EXIT_SUCCESS;
}
