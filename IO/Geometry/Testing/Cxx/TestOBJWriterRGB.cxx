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

vtkSmartPointer<vtkPolyData> CreateTestData()
{
  auto polyData = vtkSmartPointer<vtkPolyData>::New();
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetThetaResolution(8);
  sphereSource->SetPhiResolution(8);
  sphereSource->Update();
  polyData->ShallowCopy(sphereSource->GetOutput());

  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(3);
  colors->SetName("RGB");

  for (vtkIdType i = 0; i < polyData->GetNumberOfPoints(); ++i)
  {
    unsigned char color[3] = { static_cast<unsigned char>(i % 255),
      static_cast<unsigned char>((i / 255) % 255),
      static_cast<unsigned char>((i / (255 * 255)) % 255) };
    colors->InsertNextTypedTuple(color);
  }

  polyData->GetPointData()->SetScalars(colors);
  polyData->GetPointData()->SetActiveScalars("RGB");

  return polyData;
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
  if (data->GetPointData()->GetNumberOfArrays() != 1)
  {
    std::cerr << "Invalid number of arrays" << std::endl;
    return EXIT_FAILURE;
  }

  // Check the presence of "Colors" array
  auto rgbArray = data->GetPointData()->GetArray("RGB");
  if (!rgbArray)
  {
    std::cerr << "Could not find Colors array" << std::endl;
    return EXIT_FAILURE;
  }
  if (rgbArray->GetNumberOfComponents() != 3)
  {
    std::cerr << "Invalid number of components for Colors array" << std::endl;
    return EXIT_FAILURE;
  }
  if (rgbArray->GetNumberOfTuples() != data->GetNumberOfPoints())
  {
    std::cerr << "Invalid number of tuples for Colors array" << std::endl;
    return EXIT_FAILURE;
  }

  // Check "Colors" array values
  for (vtkIdType i = 0; i < data->GetNumberOfPoints(); ++i)
  {
    unsigned char color[3];
    rgbArray->GetTypedTuple(i, color);
    unsigned char expectedColor[3] = { static_cast<unsigned char>(i % 255),
      static_cast<unsigned char>((i / 255) % 255),
      static_cast<unsigned char>((i / (255 * 255)) % 255) };
    if (color[0] != expectedColor[0] || color[1] != expectedColor[1] ||
      color[2] != expectedColor[2])
    {
      std::cerr << "Invalid color value at index " << i << ": (" << (int)color[0] << ", "
                << (int)color[1] << ", " << (int)color[2] << ") instead of ("
                << (int)expectedColor[0] << ", " << (int)expectedColor[1] << ", "
                << (int)expectedColor[2] << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestOBJReaderWriterRGB(int argc, char* argv[])
{
  char* tname =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tmpDir(tname);
  delete[] tname;
  std::string filename = tmpDir + "/TestOBJReaderWriterRGB_rw.obj";

  // write test data to file by vtkOBJWriter with ColorModeOn
  auto polydata = CreateTestData();
  vtkNew<vtkOBJWriter> writer;
  writer->SetFileName(filename.data());
  writer->SetArrayName("RGB");
  writer->ColorModeOn();
  writer->SetInputData(0, polydata);
  writer->Write();

  // read this file and compare with input by vtkOBJReader
  vtkNew<vtkOBJReader> reader;
  reader->SetFileName(filename.data());
  reader->Update();
  data = reader->GetOutput();

  if (CheckData(data) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
