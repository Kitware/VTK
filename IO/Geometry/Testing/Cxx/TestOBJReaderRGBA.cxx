// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellData.h"
#include "vtkCubeSource.h"
#include "vtkDataArray.h"
#include "vtkNew.h"
#include "vtkOBJReader.h"
#include "vtkOBJWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"

#include <iostream>

namespace
{

vtkSmartPointer<vtkPolyData> CreateTestData()
{
  auto polyData = vtkSmartPointer<vtkPolyData>::New();
  vtkNew<vtkCubeSource> cubeSource;
  cubeSource->SetXLength(1.0);
  cubeSource->SetYLength(2.0);
  cubeSource->SetZLength(3.0);
  cubeSource->Update();
  polyData->ShallowCopy(cubeSource->GetOutput());
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(4);
  colors->SetName("RGBA");

  for (vtkIdType i = 0; i < polyData->GetNumberOfPoints(); ++i)
  {
    auto c = static_cast<unsigned char>((i << 6) % 255);
    unsigned char color[4] = { c, c, c, c };
    colors->InsertNextTypedTuple(color);
  }

  polyData->GetPointData()->SetScalars(colors);
  polyData->GetPointData()->SetActiveScalars("RGBA");

  return polyData;
}

void WriteTestData(const std::string& filename)
{
  auto polydata = CreateTestData();
  vtkNew<vtkOBJWriter> writer;
  writer->SetFileName(filename.data());
  writer->SetArrayName("RGBA");
  writer->ColorModeOn();
  writer->SetInputData(0, polydata);
  writer->Write();
}

int CheckData(vtkPolyData* data)
{
  if (!data)
  {
    std::cerr << "Could not read data" << std::endl;
    return EXIT_FAILURE;
  }

  // The output must have only one arrays, "Colors"

  // Check the number of arrays
  if (data->GetPointData()->GetNumberOfArrays() <= 0)
  {
    std::cerr << "Invalid number of arrays" << std::endl;
    return EXIT_FAILURE;
  }

  // Check the presence of "Colors" array
  auto rgbaArray = data->GetPointData()->GetArray("RGBA");
  if (!rgbaArray)
  {
    std::cerr << "Could not find Colors array" << std::endl;
    return EXIT_FAILURE;
  }
  if (rgbaArray->GetNumberOfComponents() != 4)
  {
    std::cerr << "Invalid number of components for Colors array" << std::endl;
    return EXIT_FAILURE;
  }
  if (rgbaArray->GetNumberOfTuples() != data->GetNumberOfPoints())
  {
    std::cerr << "Invalid number of tuples for Colors array" << std::endl;
    return EXIT_FAILURE;
  }

  // convert vtkDataArray to vtkUnsignedCharArray
  auto array = vtkArrayDownCast<vtkUnsignedCharArray>(rgbaArray);
  if (!array)
  {
    std::cerr << "RGBA array is not of type vtkUnsignedCharArray" << std::endl;
    return EXIT_FAILURE;
  }

  // Check "Colors" array values
  for (vtkIdType i = 0; i < data->GetNumberOfPoints(); ++i)
  {
    unsigned char color[4];
    array->GetTypedTuple(i, color);
    auto c = static_cast<unsigned char>((i << 6) % 255);
    unsigned char expectedColor[4] = { c, c, c, c };
    if (color[0] != expectedColor[0] || color[1] != expectedColor[1] ||
      color[2] != expectedColor[2] || color[3] != expectedColor[3])
    {
      std::cerr << "Invalid color value at index " << i << ": (" << (int)color[0] << ", "
                << (int)color[1] << ", " << (int)color[2] << ", " << (int)color[3]
                << ") instead of (" << (int)expectedColor[0] << ", " << (int)expectedColor[1]
                << ", " << (int)expectedColor[2] << ", " << (int)expectedColor[3] << ")"
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

}

//------------------------------------------------------------------------------
int TestOBJReaderRGBA(int argc, char* argv[])
{
  char* tname =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tmpDir(tname);
  delete[] tname;
  std::string filename = tmpDir + "/TestOBJReaderRGBA_rw.obj";

  WriteTestData(filename);

  // read this file and compare with input by vtkOBJReader
  vtkNew<vtkOBJReader> reader;
  reader->SetFileName(filename.data());
  reader->Update();
  auto data = reader->GetOutput();

  if (CheckData(data) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
