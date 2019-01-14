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
// .NAME Test of vtkPLYWriter
// .SECTION Description
//

#include "vtkPLYWriter.h"

#include "vtkFloatArray.h"
#include "vtkPLYReader.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkTestUtilities.h"

#include <cmath>
#include <limits>

int TestPLYWriter(int argc, char *argv[])
{
  // Test temporary directory
  char *tempDir = vtkTestUtilities::GetArgOrEnvOrDefault(
     "-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete[] tempDir;

  std::string outputfile = testDirectory + std::string("/") + std::string("tmp.ply");

  // Read file name.
  const char* inputfile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/squareTextured.ply");

  // Test if the reader thinks it can open the input file.
  if (0 == vtkPLYReader::CanReadFile(inputfile))
  {
    std::cout << "The PLY reader can not read the input file." << std::endl;
    return EXIT_FAILURE;
  }

  // Create the reader.
  vtkSmartPointer<vtkPLYReader> reader = vtkSmartPointer<vtkPLYReader>::New();
  reader->SetFileName(inputfile);
  reader->Update();
  delete [] inputfile;

  // Data to compare.
  vtkSmartPointer<vtkPolyData> data = vtkSmartPointer<vtkPolyData>::New();
  data->DeepCopy(reader->GetOutput());

  // Create the writer.
  vtkSmartPointer<vtkPLYWriter> writer = vtkSmartPointer<vtkPLYWriter>::New();
  writer->SetFileName(outputfile.c_str());
  writer->SetFileTypeToASCII();
  writer->SetTextureCoordinatesNameToTextureUV();
  writer->SetInputConnection(reader->GetOutputPort());
  writer->AddComment("TextureFile vtk.png");
  writer->Write();

  // Test if the reader thinks it can open the written file.
  if (0 == vtkPLYReader::CanReadFile(outputfile.c_str()))
  {
    std::cout << "The PLY reader can not read the written file." << std::endl;
    return EXIT_FAILURE;
  }
  reader->SetFileName(outputfile.c_str());
  reader->Update();

  vtkPolyData *newData = reader->GetOutput();

  const vtkIdType nbrPoints = newData->GetNumberOfPoints();
  if (nbrPoints != data->GetNumberOfPoints())
  {
    std::cout << "Different number of points." << std::endl;
    return EXIT_FAILURE;
  }

  vtkDataArray *tCoords = newData->GetPointData()->GetTCoords();
  if (!tCoords || !data->GetPointData()->GetTCoords())
  {
    std::cout << "Texture coordinates are not present." << std::endl;
    return EXIT_FAILURE;
  }

  const vtkIdType nbrCoords = tCoords->GetNumberOfTuples()*tCoords->GetNumberOfComponents();
  if (nbrCoords != (2 * nbrPoints))
  {
    std::cout << "Number of texture coordinates is not coherent." << std::endl;
    return EXIT_FAILURE;
  }

  vtkFloatArray * inputArray = vtkArrayDownCast<vtkFloatArray>(data->GetPointData()->GetTCoords());
  vtkFloatArray * outputArray = vtkArrayDownCast<vtkFloatArray>(tCoords);
  if (!inputArray || !outputArray)
  {
    std::cout << "Texture coordinates are not of float type." << std::endl;
    return EXIT_FAILURE;
  }

  float *input = inputArray->GetPointer(0);
  float *output = outputArray->GetPointer(0);
  for (vtkIdType id = 0; id < nbrCoords; ++id)
  {
    if (std::abs(*input++ - *output++) > std::numeric_limits<float>::epsilon())
    {
      std::cout << "Texture coordinates are not identical." << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
