// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkFileResourceStream.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkSTLReader.h"
#include "vtkTestUtilities.h"

#include <string>

namespace
{

// Check the header recovery
// specifically scalar name and number of solids
bool CheckHeader(vtkSTLReader* reader)
{
  // Check if header and solids match for ASCII STL
  bool checkPass = true;
  if (reader->GetBinaryHeader() == nullptr) // check if ASCII
  {
    reader->ScalarTagsOn();
    reader->Update();

    double range[2];
    reader->GetOutput()->GetCellData()->GetScalars("STLSolidLabeling")->GetRange(range);
    int nSolids = static_cast<int>(range[1]) + 1;

    int nHeaders = 1; // At least one solid even when it does not have associated name

    std::string header(reader->GetHeader());
    if (!header.empty())
    {
      nHeaders += std::count(header.begin(), header.end(), '\n');
    }

    checkPass = (nSolids == nHeaders);
    vtkLogIf(ERROR, !checkPass,
      "Number of Solid Names in Header does not match with the number of solids");

    reader->ScalarTagsOff();
    reader->Update();
  }

  return checkPass;
}

// Check the produced polydata
// against a vtkhdf baseline
bool CheckPolyData(int argc, char* argv[], vtkSTLReader* reader, const std::string& suffix)
{
  std::string data = "/Data/BaselineData/TestSTLReader_" + suffix + ".vtkhdf";
  bool checkPass = vtkTestUtilities::RegressionTest(argc, argv, reader->GetOutput(), data);
  vtkLogIfF(ERROR, !checkPass, "%s cannot be validated", data.c_str());
  return checkPass;
}

// Test with a filename
bool TestFile(int argc, char* argv[], const std::string& filename, const std::string& suffix)
{
  vtkLogScopeF(INFO, "TestFile %s", suffix.c_str());

  char* filepath = vtkTestUtilities::ExpandDataFileName(argc, argv, filename.c_str());
  if (!vtkSTLReader::CanReadFile(filepath))
  {
    vtkLog(ERROR, "Unexpected CanReadFile result: " << filepath);
    return false;
  }

  vtkNew<vtkSTLReader> reader;
  reader->SetFileName(filepath);
  reader->Update();
  delete[] filepath;

  bool testPass = CheckHeader(reader);
  testPass &= CheckPolyData(argc, argv, reader, suffix);
  return testPass;
}

// Test with a stream
bool TestStream(int argc, char* argv[], const std::string& filename, const std::string& suffix)
{
  vtkLogScopeF(INFO, "TestStream %s", suffix.c_str());

  char* filepath = vtkTestUtilities::ExpandDataFileName(argc, argv, filename.c_str());
  vtkNew<vtkFileResourceStream> stream;
  stream->Open(filepath);

  if (!vtkSTLReader::CanReadFile(stream))
  {
    vtkLog(ERROR, "Unexpected CanReadFile result: " << filepath);
    return false;
  }
  delete[] filepath;

  vtkNew<vtkSTLReader> reader;
  reader->SetStream(stream);
  reader->Update();

  bool testPass = CheckHeader(reader);
  testPass &= CheckPolyData(argc, argv, reader, suffix);
  return testPass;
}
}

int TestSTLReader(int argc, char* argv[])
{
  bool ret = ::TestFile(argc, argv, "Data/42400-IDGH.stl", "standard");
  ret &= ::TestStream(argc, argv, "Data/42400-IDGH.stl", "standard");

  ret &= ::TestFile(argc, argv, "Data/multiple_patches.stl", "multiple_patches");
  ret &= ::TestStream(argc, argv, "Data/multiple_patches.stl", "multiple_patches");

  // Ascii file with leading spaces on all lines, including "solid"
  ret &= ::TestFile(argc, argv, "Data/box_leading_spaces.stl", "leading_spaces");
  ret &= ::TestStream(argc, argv, "Data/box_leading_spaces.stl", "leading_spaces");

  // Binary file that starts with "solid"
  ret &= ::TestFile(argc, argv, "Data/box_binary_solid.stl", "binary_solid");
  ret &= ::TestStream(argc, argv, "Data/box_binary_solid.stl", "binary_solid");

  // This binary STL file is malformed in 2 ways:
  // - it has the 80 byte header, but it incorrectly starts with the ASCII keyword `solid`
  // - it claims to have 30886 triangles, but the file is too short and actually has just 1 triangle
  ret &= ::TestFile(argc, argv, "Data/malformed_binary.stl", "malformed_binary");
  ret &= ::TestStream(argc, argv, "Data/malformed_binary.stl", "malformed_binary");

  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
