// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkDelimitedTextReader.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTestUtilities.h>

namespace
{
//------------------------------------------------------------------------------
bool CheckOutput(vtkTable* table, int nbCols, int nbRows)
{
  if (table->GetNumberOfRows() != nbRows)
  {
    vtkLog(ERROR, "Wrong number of rows: " << table->GetNumberOfRows() << endl);
    return false;
  }
  if (table->GetNumberOfColumns() != nbCols)
  {
    vtkLog(ERROR, "Wrong number of columns: " << table->GetNumberOfColumns());
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestReadFromString()
{
  std::string inputString =
    ",awesomeness,fitness,region\r\nAbby,1,2,china\r\nBob,5,0.2,US\r\nCatie,3,0."
    "3,UK\r\nDavid,2,100,UK\r\nGrace,4,20,US\r\nIlknur,6,5,Turkey\r\n";
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetHaveHeaders(true);
  reader->SetReadFromInputString(true);
  reader->SetInputString(inputString);
  reader->Update();

  return CheckOutput(reader->GetOutput(), 4, 6);
}

//------------------------------------------------------------------------------
bool TestDefault(int argc, char* argv[])
{
  char* filepath = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/authors.csv");
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(filepath);
  reader->Update();
  delete[] filepath;

  return CheckOutput(reader->GetOutput(), 6, 7);
}

//------------------------------------------------------------------------------
bool TestHeaders(int argc, char* argv[])
{
  char* filepath = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/authors.csv");
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(filepath);
  reader->Update();
  delete[] filepath;

  bool ret = CheckOutput(reader->GetOutput(), 6, 7);

  reader->SetHaveHeaders(true);
  reader->Update();
  ret &= CheckOutput(reader->GetOutput(), 6, 6);

  return ret;
}

//------------------------------------------------------------------------------
bool TestDelimiters(int argc, char* argv[])
{
  char* filepath = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/delimited.txt");
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(filepath);
  reader->SetHaveHeaders(true);
  reader->Update();
  delete[] filepath;

  bool ret = CheckOutput(reader->GetOutput(), 1, 5);

  reader->SetFieldDelimiterCharacters(":");
  reader->Update();
  vtkTable* table = reader->GetOutput();
  ret &= CheckOutput(table, 4, 5);

  auto column = table->GetColumnByName("My Field Name 2");
  if (!column)
  {
    vtkLog(ERROR, "ERROR: column <My Field Name 2> not found.\n");
  }
  auto stringCol = vtkStringArray::SafeDownCast(column);
  std::string data = stringCol->GetValue(2);
  if (data != "String:Delimiters")
  {
    vtkLog(ERROR, "ERROR: string delimiter failed. Has <" << data << "> \n");
  }

  // merging delimiters
  filepath = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/delimited2.txt");
  reader->SetFileName(filepath);
  reader->SetFieldDelimiterCharacters(",");
  reader->MergeConsecutiveDelimitersOn();
  reader->Update();
  delete[] filepath;

  ret &= CheckOutput(reader->GetOutput(), 9, 1);

  column = table->GetColumnByName("Sam");
  if (!column)
  {
    vtkLog(ERROR, "ERROR: column <Sam> not found.\n");
  }
  stringCol = vtkStringArray::SafeDownCast(column);
  data = stringCol->GetValue(0);
  if (data != "line")
  {
    vtkLog(ERROR, "ERROR: string delimiter failed. Has <" << data << "> \n");
  }

  return ret;
}

//------------------------------------------------------------------------------
bool TestNumericsDefaultToString()
{
  std::string inputString = "Int,Str,Double\n";
  inputString += "1,_a2_,3.1";
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetHaveHeaders(true);
  reader->SetStringDelimiter('_');
  reader->SetReadFromInputString(true);
  reader->SetInputString(inputString);
  reader->Update();

  vtkNew<vtkStringArray> Int;
  Int->SetName("Int");
  Int->InsertNextValue("1");
  vtkNew<vtkStringArray> Str;
  Str->SetName("Str");
  Str->InsertNextValue("a2");
  vtkNew<vtkStringArray> Double;
  Double->SetName("Double");
  Double->InsertNextValue("3.1");
  vtkNew<vtkTable> expectedTable;
  expectedTable->SetNumberOfRows(1);
  expectedTable->AddColumn(Int);
  expectedTable->AddColumn(Str);
  expectedTable->AddColumn(Double);

  vtkTable* output = reader->GetOutput();

  return vtkTestUtilities::CompareDataObjects(output, expectedTable);
}

//------------------------------------------------------------------------------
bool TestNumericsDetectType()
{
  std::string inputString = "Int,Str,Double\n";
  inputString += "1,a2,3.1\n";
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetHaveHeaders(true);
  reader->SetReadFromInputString(true);
  reader->SetInputString(inputString);
  reader->SetDetectNumericColumns(true);
  reader->Update();

  vtkNew<vtkIntArray> Int;
  Int->SetName("Int");
  Int->InsertNextValue(1);
  vtkNew<vtkStringArray> Str;
  Str->SetName("Str");
  Str->InsertNextValue("a2");
  vtkNew<vtkDoubleArray> Double;
  Double->SetName("Double");
  Double->InsertNextValue(3.1);
  vtkNew<vtkTable> expectedTable;
  expectedTable->AddColumn(Int);
  expectedTable->AddColumn(Str);
  expectedTable->AddColumn(Double);

  vtkTable* output = reader->GetOutput();

  return vtkTestUtilities::CompareDataObjects(output, expectedTable);
}

//------------------------------------------------------------------------------
bool TestNumericsConvertType()
{
  std::string inputString = "IntToDouble,IntToStr,Double\n";
  inputString += "1,2,3.1\n";
  // second col becomes string
  inputString += "1,_a2_,3.1\n";
  // first col becomes double
  inputString += "1.1,2.2,3\n";
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetHaveHeaders(true);
  reader->SetStringDelimiter('_');
  reader->SetReadFromInputString(true);
  reader->SetInputString(inputString);
  reader->SetDetectNumericColumns(true);

  reader->Update();

  vtkNew<vtkDoubleArray> IntToDouble;
  IntToDouble->SetName("IntToDouble");
  IntToDouble->InsertNextValue(1);
  IntToDouble->InsertNextValue(1);
  IntToDouble->InsertNextValue(1.1);
  vtkNew<vtkStringArray> IntToStr;
  IntToStr->SetName("IntToStr");
  IntToStr->InsertNextValue("2");
  IntToStr->InsertNextValue("a2");
  IntToStr->InsertNextValue("2.2");
  vtkNew<vtkDoubleArray> Double;
  Double->SetName("Double");
  Double->InsertNextValue(3.1);
  Double->InsertNextValue(3.1);
  Double->InsertNextValue(3.);
  vtkNew<vtkTable> expectedTable;
  expectedTable->AddColumn(IntToDouble);
  expectedTable->AddColumn(IntToStr);
  expectedTable->AddColumn(Double);

  vtkTable* output = reader->GetOutput();

  return vtkTestUtilities::CompareDataObjects(output, expectedTable);
}

//------------------------------------------------------------------------------
bool TestNumericsOverflow()
{
  std::string inputString = "Int,Int1,Double\n";
  inputString += "1,2,3.1\n";
  inputString += "1234567890123,-1234567890123,3.1e7\n";
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetHaveHeaders(true);
  reader->SetReadFromInputString(true);
  reader->SetInputString(inputString);
  reader->SetDetectNumericColumns(true);
  reader->Update();

  vtkNew<vtkDoubleArray> Int;
  Int->SetName("Int");
  Int->InsertNextValue(1);
  Int->InsertNextValue(1234567890123);
  vtkNew<vtkDoubleArray> Int1;
  Int1->SetName("Int1");
  Int1->InsertNextValue(2);
  Int1->InsertNextValue(-1234567890123);
  vtkNew<vtkDoubleArray> Double;
  Double->SetName("Double");
  Double->InsertNextValue(3.1);
  Double->InsertNextValue(3.1e7);
  vtkNew<vtkTable> expectedTable;
  expectedTable->AddColumn(Int);
  expectedTable->AddColumn(Int1);
  expectedTable->AddColumn(Double);

  vtkTable* output = reader->GetOutput();
  return vtkTestUtilities::CompareDataObjects(output, expectedTable);
}

//------------------------------------------------------------------------------
bool TestNumerics()
{
  if (!::TestNumericsDefaultToString())
  {
    vtkLog(ERROR, "Test default to string failed.\n");
    return false;
  }
  else if (!::TestNumericsDetectType())
  {
    vtkLog(ERROR, "Test column type detection failed.\n");
    return false;
  }
  else if (!::TestNumericsConvertType())
  {
    vtkLog(ERROR, "Test column type conversion failed.\n");
    return false;
  }
  else if (!::TestNumericsOverflow())
  {
    vtkLog(ERROR, "Test overflow failed.\n");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestCharSets(int argc, char* argv[])
{
  char* filepath = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/delimitedUTF16LE.txt");
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(filepath);
  reader->SetHaveHeaders(true);
  reader->SetFieldDelimiterCharacters(":");
  reader->SetUnicodeCharacterSet("UTF-16LE");
  reader->Update();
  delete[] filepath;

  bool ret = CheckOutput(reader->GetOutput(), 4, 5);

  filepath = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/delimitedUTF16BE.txt");
  reader->SetFileName(filepath);
  reader->SetUnicodeCharacterSet("UTF-16BE");
  reader->Update();
  delete[] filepath;

  ret &= CheckOutput(reader->GetOutput(), 4, 5);

  return ret;
}

bool TestPreview(int argc, char* argv[])
{
  char* filepath = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/authors.csv");
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(filepath);
  delete[] filepath;

  reader->UpdateInformation();
  std::string preview = reader->GetPreview();
  if (!preview.empty())
  {
    vtkLog(ERROR, "Preview should be empty by default, has: \n" << preview);
  }

  std::string firstlines = "Author,Affiliation,Alma Mater,Categories,Age,Coolness\r\n";

  reader->SetPreviewNumberOfLines(1);
  reader->UpdateInformation();
  preview = reader->GetPreview();
  if (preview.empty())
  {
    vtkLog(ERROR, "Preview should contains first line, but is empty");
    return false;
  }

  if (preview != firstlines)
  {
    vtkLog(ERROR,
      "Preview wrong first line. Has: <" << preview << ">"
                                         << "But expect <" << firstlines << ">");
    return false;
  }

  firstlines += "Biff,NASA,Ole Southern,Jazz; Rocket Science,27,0.6\r\n";
  firstlines += "Bob,Bob's Supermarket,Ole Southern,Jazz,54,0.3\r\n";

  reader->SetPreviewNumberOfLines(3);
  reader->UpdateInformation();
  preview = reader->GetPreview();

  if (preview != firstlines)
  {
    vtkLog(ERROR, "Preview wrong contents. Has: <" << preview << ">");
    return false;
  }

  return true;
}

bool TestSkipLines(int argc, char* argv[])
{
  char* filepath = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/authors.csv");
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(filepath);
  delete[] filepath;

  reader->SetSkippedRecords(3);
  reader->Update();
  // skip header lines

  return CheckOutput(reader->GetOutput(), 6, 4);
}

bool TestComments(int argc, char* argv[])
{
  char* filepath =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/half_sphere_commented.csv");
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(filepath);
  delete[] filepath;
  reader->SetHaveHeaders(true);
  reader->Update();
  vtkTable* outTable = reader->GetOutput();

  // check the line with ending comment
  vtkVariant variant = outTable->GetValueByName(2, "RandomPointScalars");
  auto value = variant.ToInt();
  if (value != 57)
  {
    vtkLog(ERROR, "Wrong value in commented line, has " << value);
    return false;
  }

  if (!CheckOutput(outTable, 7, 50))
  {
    return false;
  }

  // add comma as comment char
  reader->SetCommentCharacters("#,");
  // use another field delimiter
  reader->SetFieldDelimiterCharacters(" ");
  reader->Update();
  outTable = reader->GetOutput();
  if (!CheckOutput(outTable, 1, 50))
  {
    return false;
  }

  vtkAbstractArray* named = outTable->GetColumnByName("Normals:0");
  vtkAbstractArray* first = outTable->GetColumn(0);
  if (!named)
  {
    vtkLog(ERROR, "Wrong name for column " << first->GetName());
    return false;
  }

  return true;
}
};

//------------------------------------------------------------------------------
int TestDelimitedTextReader(int argc, char* argv[])
{
  if (!::TestDefault(argc, argv))
  {
    vtkLog(ERROR, "Test Default failed.\n");
  }
  else if (!::TestHeaders(argc, argv))
  {
    vtkLog(ERROR, "Test Headers failed\n");
  }
  else if (!::TestDelimiters(argc, argv))
  {
    vtkLog(ERROR, "Test Delimiters failed.\n");
  }
  else if (!::TestReadFromString())
  {
    vtkLog(ERROR, "Test Read From String failed.\n");
  }
  else if (!::TestCharSets(argc, argv))
  {
    vtkLog(ERROR, "Test CharSets failed.\n");
  }
  else if (!::TestNumerics())
  {
    vtkLog(ERROR, "Test Numerics failed.\n");
  }
  else if (!::TestPreview(argc, argv))
  {
    vtkLog(ERROR, "Test Preview failed.\n");
  }
  else if (!::TestSkipLines(argc, argv))
  {
    vtkLog(ERROR, "Test SkipLines failed.\n");
  }
  else if (!::TestComments(argc, argv))
  {
    vtkLog(ERROR, "Test Comments failed.\n");
  }
  else
  {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
