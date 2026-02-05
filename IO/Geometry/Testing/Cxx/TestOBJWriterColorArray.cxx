// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellData.h"
#include "vtkNew.h"
#include "vtkOBJReader.h"
#include "vtkOBJWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

#include <iostream>

namespace
{

template <const int N>
auto CreateTestColor(int index) -> std::array<unsigned char, N>
{
  auto c = static_cast<unsigned char>((index << 4) % 255);
  std::array<unsigned char, N> color;
  color.fill(c);
  return color;
}

template <int N>
vtkSmartPointer<vtkPolyData> CreateTestData(const char* ArrayName)
{
  auto polyData = vtkSmartPointer<vtkPolyData>::New();
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetThetaResolution(8);
  sphereSource->SetPhiResolution(8);
  sphereSource->Update();
  polyData->ShallowCopy(sphereSource->GetOutput());

  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(N);
  colors->SetName(ArrayName);

  for (vtkIdType i = 0; i < polyData->GetNumberOfPoints(); ++i)
  {
    auto color = CreateTestColor<N>(i);
    colors->InsertNextTypedTuple(color.data());
  }

  polyData->GetPointData()->SetScalars(colors);
  polyData->GetPointData()->SetActiveScalars(ArrayName);

  return polyData;
}

template <int N>
void WriteTestData(const std::string& filename, const std::string& arrayName)
{
  auto polydata = CreateTestData<N>(arrayName.data());
  vtkNew<vtkOBJWriter> writer;
  writer->SetFileName(filename.data());
  writer->SetColorArrayName(arrayName);
  writer->WriteColorArrayOn();
  writer->SetInputData(0, polydata);
  writer->Write();
}

template <int N>
int CheckData(vtkPolyData* data, const std::string& arrayName)
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
  auto colorArray = data->GetPointData()->GetArray(arrayName.data());
  if (!colorArray)
  {
    std::cerr << "Could not find Colors array" << std::endl;
    return EXIT_FAILURE;
  }
  if (colorArray->GetNumberOfComponents() != N)
  {
    std::cerr << "Invalid number of components for Colors array" << std::endl;
    return EXIT_FAILURE;
  }
  if (colorArray->GetNumberOfTuples() != data->GetNumberOfPoints())
  {
    std::cerr << "Invalid number of tuples for Colors array" << std::endl;
    return EXIT_FAILURE;
  }

  // convert vtkDataArray to vtkUnsignedCharArray
  auto array = vtkArrayDownCast<vtkUnsignedCharArray>(colorArray);
  if (!array)
  {
    std::cerr << "Color array is not of type vtkUnsignedCharArray" << std::endl;
    return EXIT_FAILURE;
  }

  // Check "Colors" array values
  for (vtkIdType i = 0; i < data->GetNumberOfPoints(); ++i)
  {
    std::array<unsigned char, N> color;
    array->GetTypedTuple(i, color.data());
    std::array<unsigned char, N> expectedColor = CreateTestColor<N>(i);
    if (!std::equal(std::begin(color), std::end(color), std::begin(expectedColor)))
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

template <int N>
bool ColorArrayWriteTest(const std::string& filename, const std::string& arrayName)
{
  WriteTestData<N>(filename, arrayName);

  // read this file and compare with input by vtkOBJReader
  vtkNew<vtkOBJReader> reader;
  reader->SetFileName(filename.data());
  reader->Update();
  auto data = reader->GetOutput();

  if (CheckData<N>(data, arrayName) == EXIT_FAILURE)
  {
    return false;
  }

  return true;
}

}

//------------------------------------------------------------------------------
int TestOBJWriterColorArray(int argc, char* argv[])
{
  char* tname =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tmpDir(tname);
  delete[] tname;

  // test RGB
  {
    const int N = 3;
    const std::string& arrayName = "RGB";
    if (!ColorArrayWriteTest<N>(tmpDir + "/TestOBJWriterColorArray_RGB_rw.obj", arrayName))
    {
      return EXIT_FAILURE;
    }
  }

  // test RGBA
  {
    const int N = 4;
    const std::string& arrayName = "RGBA";

    if (!ColorArrayWriteTest<N>(tmpDir + "/TestOBJWriterColorArray_RGBA_rw.obj", arrayName))
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
