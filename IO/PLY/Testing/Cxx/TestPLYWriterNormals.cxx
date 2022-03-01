/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYWriterNormals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of TestPLYWriterNormals
// .SECTION Description
// Tests if PLY writer saves point normals in the output file if and only if
// point normals are available in the input mesh.

#include "vtkPLYWriter.h"

#include "vtkFloatArray.h"
#include "vtkPLYReader.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"
#include "vtksys/FStream.hxx"

#include "vtkPolyDataNormals.h"
#include "vtkSphereSource.h"

#include <cmath>
#include <fstream>
#include <limits>
#include <streambuf>

int TestPLYWriterNormals(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete[] tempDir;

  // Test mesh writing with normals

  std::string filename = testDirectory + std::string("/TestPlyWriterNormalsOutput.ply");

  vtkNew<vtkSphereSource> sphere;
  sphere->GenerateNormalsOn();

  vtkNew<vtkPLYWriter> writer;
  writer->SetInputConnection(sphere->GetOutputPort());
  writer->SetFileName(filename.c_str());
  writer->Write();

  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  vtkPolyData* pd = reader->GetOutput();
  if (!pd->GetPointData()->GetNormals())
  {
    std::cout << "Not found normals in a ply file that is expected to contain normals.\n";
    return EXIT_FAILURE;
  }

  // Test mesh writing without normals

  filename = testDirectory + std::string("/TestPlyWriterNoNormalsOutput.ply");
  sphere->GenerateNormalsOff();
  writer->SetFileName(filename.c_str());
  writer->Write();

  reader->SetFileName(filename.c_str());
  reader->Update();
  pd = reader->GetOutput();
  if (pd->GetPointData() && pd->GetPointData()->GetNormals())
  {
    std::cout << "Found normals in a ply file that is expected not to contain normals.\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
