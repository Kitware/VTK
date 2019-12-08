/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOBJReaderMultiTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDebugLeaks.h"
#include "vtkOBJReader.h"

#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkTestUtilities.h"

//------------------------------------------------------------------------------
int TestOBJReaderMultiTexture(int argc, char* argv[])
{
  // Create the reader.
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/obj_multitexture.obj");

  vtkNew<vtkOBJReader> reader;
  reader->SetFileName(fname);
  reader->Update();

  vtkPolyData* data = reader->GetOutput();

  delete[] fname;

  if (!data)
  {
    std::cerr << "Could not read data" << std::endl;
    return EXIT_FAILURE;
  }

  // The OBJ file has 3 cells and 8 points.
  // 4 of those points have 2 textures associated, thus the reader output
  // must have 12 points.
  if (data->GetNumberOfPoints() != 12 && data->GetNumberOfCells() == 3)
  {
    std::cerr << "Invalid number of points or cells" << std::endl;
    return EXIT_FAILURE;
  }

  // The output must have 2 arrays, texture_0 & texture_1. texture_0 has
  // (-1, -1) for indices 4 to 7. texture_1 has (-1, -1) for every indices but
  // those between 4 and 7

  // Check the number of arrays
  if (data->GetPointData()->GetNumberOfArrays() != 2)
  {
    std::cerr << "Invalid number of arrays" << std::endl;
    return EXIT_FAILURE;
  }

  vtkDataArray* texture0 = data->GetPointData()->GetArray("texture_0");
  vtkDataArray* texture1 = data->GetPointData()->GetArray("texture_1");
  // Check if the arrays are named correctly
  if (!texture0)
  {
    std::cerr << "Could not find texture_0 array" << std::endl;
    return EXIT_FAILURE;
  }

  if (!texture1)
  {
    std::cerr << "Could not find texture_1 array" << std::endl;
    return EXIT_FAILURE;
  }

  // Check the values
  for (int i = 0; i < 12; ++i)
  {
    double* currentTCoord0 = texture0->GetTuple2(i);
    double* currentTCoord1 = texture1->GetTuple2(i);

    // Testing values outside [4, 7]
    if (i < 4 || i > 7)
    {
      if ((currentTCoord0[0] == -1 && currentTCoord0[1] == -1) ||
        !(currentTCoord1[0] == -1 && currentTCoord1[1] == -1))
      {
        std::cerr << "Unexpected texture values" << std::endl;
        return EXIT_FAILURE;
      }
    }
    // Testing values inside [4, 7]
    else
    {
      if (!(currentTCoord0[0] == -1 && currentTCoord0[1] == -1) ||
        (currentTCoord1[0] == -1 && currentTCoord1[1] == -1))
      {
        std::cerr << "Unexpected texture values" << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
