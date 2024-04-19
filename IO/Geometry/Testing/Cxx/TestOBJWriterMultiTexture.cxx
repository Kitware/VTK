// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellData.h"
#include "vtkDebugLeaks.h"
#include "vtkNew.h"
#include "vtkOBJReader.h"
#include "vtkOBJWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"

int CheckData(vtkPolyData* data)
{
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

  // Check the texture values
  for (int i = 0; i < 12; ++i)
  {
    double* currentTCoord0 = texture0->GetTuple2(i);
    double* currentTCoord1 = texture1->GetTuple2(i);

    // Testing values < 4
    if (i < 4)
    {
      if (!(currentTCoord0[0] == -1 && currentTCoord0[1] == -1) ||
        !(currentTCoord1[0] == -1 && currentTCoord1[1] == -1))
      {
        std::cerr << "Unexpected texture values" << std::endl;
        return EXIT_FAILURE;
      }
    }
    // Testing values inside [4, 7]
    else if (i >= 4 && i <= 7)
    {
      if (!(currentTCoord0[0] == -1 && currentTCoord0[1] == -1) ||
        (currentTCoord1[0] == -1 && currentTCoord1[1] == -1))
      {
        std::cerr << "Unexpected texture values" << std::endl;
        return EXIT_FAILURE;
      }
    }
    // Testing values > 7
    else if (i > 7)
    {
      if ((currentTCoord0[0] == -1 && currentTCoord0[1] == -1) ||
        !(currentTCoord1[0] == -1 && currentTCoord1[1] == -1))
      {
        std::cerr << "Unexpected texture values" << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  vtkIntArray* materialIds =
    vtkIntArray::SafeDownCast(data->GetCellData()->GetArray("MaterialIds"));
  if (!(materialIds && materialIds->GetNumberOfTuples() == 3 && materialIds->GetValue(0) == 0 &&
        materialIds->GetValue(1) && materialIds->GetValue(2) == 2))
  {
    std::cerr << "Invalid MaterialIds cell array" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestOBJWriterMultiTexture(int argc, char* argv[])
{
  // Create the reader.
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/obj_multitexture_notexture.obj");

  vtkNew<vtkOBJReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  vtkPolyData* data = reader->GetOutput();
  delete[] fname;

  if (CheckData(data) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  char* tname =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tmpDir(tname);
  delete[] tname;
  std::string filename = tmpDir + "/TestOBJWriterMultiTexture_write.obj";

  vtkNew<vtkOBJWriter> writer;
  writer->SetFileName(filename.c_str());
  writer->SetInputConnection(0, reader->GetOutputPort());
  writer->Write();

  // read this file and compare with input
  vtkNew<vtkOBJReader> reader2;
  reader2->SetFileName(filename.c_str());
  reader2->Update();
  data = reader2->GetOutput();

  if (CheckData(data) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
