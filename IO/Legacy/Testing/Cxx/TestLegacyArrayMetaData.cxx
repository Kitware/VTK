/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLegacyArrayMetaData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Roundtrip test for array metadata in legacy readers.

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkInformationKey.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkUnstructuredGridWriter.h"

// Serializable keys to test:
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationUnsignedLongKey.h"

namespace {

static vtkInformationDoubleKey *TestDoubleKey =
    vtkInformationDoubleKey::MakeKey("Double", "TestKey");
// Test restricted keys with this one -- must be a vector of length 3, can NOT
// be constructed using Append():
static vtkInformationDoubleVectorKey *TestDoubleVectorKey =
    vtkInformationDoubleVectorKey::MakeKey("DoubleVector", "TestKey", 3);
static vtkInformationIdTypeKey *TestIdTypeKey =
    vtkInformationIdTypeKey::MakeKey("IdType", "TestKey");
static vtkInformationIntegerKey *TestIntegerKey =
    vtkInformationIntegerKey::MakeKey("Integer", "TestKey");
static vtkInformationIntegerVectorKey *TestIntegerVectorKey =
    vtkInformationIntegerVectorKey::MakeKey("IntegerVector", "TestKey");
static vtkInformationStringKey *TestStringKey =
    vtkInformationStringKey::MakeKey("String", "TestKey");
static vtkInformationStringVectorKey *TestStringVectorKey =
    vtkInformationStringVectorKey::MakeKey("StringVector", "TestKey");
static vtkInformationUnsignedLongKey *TestUnsignedLongKey =
    vtkInformationUnsignedLongKey::MakeKey("UnsignedLong", "TestKey");

bool stringEqual(const std::string &expect, const std::string &actual)
{
  if (expect != actual)
  {
    std::cerr << "Strings do not match! Expected: '" << expect << "', got: '"
              << actual << "'.\n";
    return false;
  }
  return true;
}

bool stringEqual(const std::string &expect, const char *actual)
{
  return stringEqual(expect, std::string(actual ? actual : ""));
}

template <typename T>
bool compareValues(const std::string &desc, T expect, T actual)
{
  if (expect != actual)
  {
    std::cerr << "Failed comparison for '" << desc << "'. Expected '" << expect
              << "', got '" << actual << "'.\n";
    return false;
  }
  return true;
}

bool verify(vtkUnstructuredGrid *grid)
{
  vtkDataArray *array = grid->GetPoints()->GetData();
  vtkInformation *info = array->GetInformation();
  if (!info)
  {
    std::cerr << "Missing information object!\n";
    return false;
  }

  if (!stringEqual("X coordinates", array->GetComponentName(0)) ||
      !stringEqual("Y coordinates", array->GetComponentName(1)) ||
      !stringEqual("Z coordinates", array->GetComponentName(2)))

  {
    return false;
  }

  if (!compareValues("double key", 1., info->Get(TestDoubleKey)) ||
      !compareValues("double vector key length", 3, info->Length(TestDoubleVectorKey)) ||
      !compareValues("double vector key @0", 1., info->Get(TestDoubleVectorKey, 0)) ||
      !compareValues("double vector key @1", 90., info->Get(TestDoubleVectorKey, 1)) ||
      !compareValues("double vector key @2", 260., info->Get(TestDoubleVectorKey, 2)) ||
      !compareValues<vtkIdType>("idtype key", 5, info->Get(TestIdTypeKey)) ||
      !compareValues("integer key", 408, info->Get(TestIntegerKey)) ||
      !compareValues("integer vector key length", 3, info->Length(TestIntegerVectorKey)) ||
      !compareValues("integer vector key @0", 1, info->Get(TestIntegerVectorKey, 0)) ||
      !compareValues("integer vector key @1", 5, info->Get(TestIntegerVectorKey, 1)) ||
      !compareValues("integer vector key @2", 45, info->Get(TestIntegerVectorKey, 2)) ||
      !stringEqual("Test String!\nLine2", info->Get(TestStringKey)) ||
      !compareValues("string vector key length", 3, info->Length(TestStringVectorKey)) ||
      !stringEqual("First", info->Get(TestStringVectorKey, 0)) ||
      !stringEqual("Second (with whitespace!)", info->Get(TestStringVectorKey, 1)) ||
      !stringEqual("Third (with\nnewline!)", info->Get(TestStringVectorKey, 2)) ||
      !compareValues("unsigned long key", 9ul, info->Get(TestUnsignedLongKey)))
  {
    return false;
  }

  array = grid->GetCellData()->GetArray("vtkGhostType");
  info = array->GetInformation();
  if (!info)
  {
    std::cerr << "Missing information object!\n";
    return false;
  }
  if (!stringEqual("Ghost level information", array->GetComponentName(0)) ||
      !stringEqual("N/A", info->Get(vtkDataArray::UNITS_LABEL())))
  {
    return false;
  }

  return true;
}
} // end anon namespace

int TestLegacyArrayMetaData(int argc, char *argv[])
{
  // Load the initial dataset:
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  std::string filename = testing->GetDataRoot();
  filename += "/Data/ghost_cells.vtk";

  vtkNew<vtkUnstructuredGridReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();
  vtkUnstructuredGrid *grid = reader->GetOutput();

  // Set component names on points:
  vtkDataArray *array = grid->GetPoints()->GetData();
  array->SetComponentName(0, "X coordinates");
  array->SetComponentName(1, "Y coordinates");
  array->SetComponentName(2, "Z coordinates");

  // Test information keys that can be serialized
  vtkInformation *info = array->GetInformation();
  info->Set(TestDoubleKey, 1.0);
  // Set the double vector using an array / Set instead of appending. Appending
  // doesn't work when RequiredLength is set.
  double doubleVecData[3] = {1., 90., 260.};
  info->Set(TestDoubleVectorKey, doubleVecData, 3);
  info->Set(TestIdTypeKey, 5);
  info->Set(TestIntegerKey, 408);
  info->Append(TestIntegerVectorKey, 1);
  info->Append(TestIntegerVectorKey, 5);
  info->Append(TestIntegerVectorKey, 45);
  info->Set(TestStringKey, "Test String!\nLine2");
  info->Append(TestStringVectorKey, "First");
  info->Append(TestStringVectorKey, "Second (with whitespace!)");
  info->Append(TestStringVectorKey, "Third (with\nnewline!)");
  info->Set(TestUnsignedLongKey, 9);

  // And on the vtkGhostLevels array:
  array = grid->GetCellData()->GetArray("vtkGhostType");
  info = array->GetInformation();
  info->Set(vtkDataArray::UNITS_LABEL(), "N/A");
  array->SetComponentName(0, "Ghost level information");

  // Check that the input grid passes our test:
  if (!verify(grid))
  {
    std::cerr << "Sanity check failed.\n";
    return EXIT_FAILURE;
  }

  // Now roundtrip the dataset through the readers/writers:
  vtkNew<vtkUnstructuredGridWriter> testWriter;
  vtkNew<vtkUnstructuredGridReader> testReader;

  testWriter->SetInputData(grid);
  testWriter->WriteToOutputStringOn();
  testReader->ReadFromInputStringOn();

  // Test ASCII mode:
  testWriter->SetFileTypeToASCII();
  if (!testWriter->Write())
  {
    std::cerr << "Write failed!" << std::endl;
    return EXIT_FAILURE;
  }

  testReader->SetInputString(testWriter->GetOutputStdString());
  testReader->Update();
  grid = testReader->GetOutput();

  if (!verify(grid))
  {
    std::cerr << "ASCII mode test failed.\n"
              << "Error while parsing:\n"
              << testWriter->GetOutputStdString() << "\n";
    return EXIT_FAILURE;
  }

  // Test binary mode:
  testWriter->SetFileTypeToBinary();
  if (!testWriter->Write())
  {
    std::cerr << "Write failed!" << std::endl;
    return EXIT_FAILURE;
  }

  testReader->SetInputString(testWriter->GetOutputStdString());
  testReader->Update();
  grid = testReader->GetOutput();

  if (!verify(grid))
  {
    std::cerr << "Binary mode test failed.\n"
              << "Error while parsing:\n"
              << testWriter->GetOutputStdString() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
