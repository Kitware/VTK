/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkPLYWriterString
// .SECTION Description
// Tests reading and writing to std::string

#include "vtkPLYWriter.h"

#include "vtkFloatArray.h"
#include "vtkPLYReader.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"
#include "vtksys/FStream.hxx"

#include <cmath>
#include <fstream>
#include <limits>
#include <streambuf>

int TestPLYWriterString(int argc, char* argv[])
{
  // Read file name.
  const char* tempFileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/squareTextured.ply");
  std::string filename = tempFileName;
  delete[] tempFileName;

  vtksys::ifstream ifs;

  ifs.open(filename.c_str(), std::ios::in | std::ios::binary);
  if (!ifs.is_open())
  {
    std::cout << "Can not read the input file." << std::endl;
    return EXIT_FAILURE;
  }
  std::string inputString{ std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>() };

  // Create the reader.
  vtkNew<vtkPLYReader> reader;
  reader->ReadFromInputStringOn();
  reader->SetInputString(inputString);
  reader->Update();

  // Data to compare.
  vtkSmartPointer<vtkPolyData> data = vtkSmartPointer<vtkPolyData>::New();
  data->DeepCopy(reader->GetOutput());

  int options[3][2] = { { VTK_ASCII, 0 }, { VTK_BINARY, VTK_BIG_ENDIAN },
    { VTK_BINARY, VTK_LITTLE_ENDIAN } };

  for (size_t i = 0; i < sizeof(options) / sizeof(options[0]); ++i)
  {
    int* option = options[i];
    // Create the writer.
    vtkNew<vtkPLYWriter> writer;
    writer->WriteToOutputStringOn();
    writer->SetFileType(option[0]);
    writer->SetDataByteOrder(option[1]);
    writer->SetTextureCoordinatesNameToTextureUV();
    writer->SetInputConnection(reader->GetOutputPort());
    writer->AddComment("TextureFile vtk.png");
    writer->Write();
    // read the written output string
    reader->SetInputString(writer->GetOutputString());
    reader->Update();

    vtkPolyData* newData = reader->GetOutput();

    const vtkIdType nbrPoints = newData->GetNumberOfPoints();
    if (nbrPoints != data->GetNumberOfPoints())
    {
      std::cout << "Different number of points." << std::endl;
      return EXIT_FAILURE;
    }

    vtkDataArray* tCoords = newData->GetPointData()->GetTCoords();
    if (!tCoords || !data->GetPointData()->GetTCoords())
    {
      std::cout << "Texture coordinates are not present." << std::endl;
      return EXIT_FAILURE;
    }

    const vtkIdType nbrCoords = tCoords->GetNumberOfTuples() * tCoords->GetNumberOfComponents();
    if (nbrCoords != (2 * nbrPoints))
    {
      std::cout << "Number of texture coordinates is not coherent." << std::endl;
      return EXIT_FAILURE;
    }

    vtkFloatArray* inputArray = vtkArrayDownCast<vtkFloatArray>(data->GetPointData()->GetTCoords());
    vtkFloatArray* outputArray = vtkArrayDownCast<vtkFloatArray>(tCoords);
    if (!inputArray || !outputArray)
    {
      std::cout << "Texture coordinates are not of float type." << std::endl;
      return EXIT_FAILURE;
    }

    float* input = inputArray->GetPointer(0);
    float* output = outputArray->GetPointer(0);
    for (vtkIdType id = 0; id < nbrCoords; ++id)
    {
      if (std::abs(*input++ - *output++) > std::numeric_limits<float>::epsilon())
      {
        std::cout << "Texture coordinates are not identical." << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
