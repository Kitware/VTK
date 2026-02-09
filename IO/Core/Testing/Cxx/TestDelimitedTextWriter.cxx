// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDelimitedTextWriter.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkTable.h"

#include <iostream>
#include <sstream>
#include <string>

int TestDelimitedTextWriter(int, char*[])
{
  vtkNew<vtkTable> table;
  vtkNew<vtkIntArray> intArray;
  intArray->SetName("Integers");
  table->AddColumn(intArray);
  vtkNew<vtkDoubleArray> doubleArray;
  doubleArray->SetName("Doubles");
  table->AddColumn(doubleArray);
  vtkNew<vtkStringArray> stringArray;
  stringArray->SetName("Strings");
  table->AddColumn(stringArray);

  for (int i = 0; i < 5; ++i)
  {
    intArray->InsertNextValue(i);
    doubleArray->InsertNextValue(i * 0.5);
    stringArray->InsertNextValue("String " + vtk::to_string(i));
  }

  vtkNew<vtkDelimitedTextWriter> writer;
  writer->SetInputData(table);
  writer->WriteToOutputStringOn();
  writer->SetFieldDelimiter(",");
  writer->SetStringDelimiter("\"");
  writer->SetUseStringDelimiter(true);
  writer->SetPrecision(2);
  writer->SetNotationToFixed();

  if (!writer->Write())
  {
    std::cerr << "Error: Failure on write." << std::endl;
    return EXIT_FAILURE;
  }

  const char* output = writer->RegisterAndGetOutputString();

  // Test the Output
  std::string outputStr(output);
  // Check that the header contains all column names
  if (outputStr.find("Integers") == std::string::npos ||
    outputStr.find("Doubles") == std::string::npos ||
    outputStr.find("Strings") == std::string::npos)
  {
    std::cerr << "Error: Output string missing column names." << std::endl;
    return EXIT_FAILURE;
  }
  // Check that there are 5 rows of data
  int rowCount = 0;
  std::istringstream iss(outputStr);
  std::string line;
  // Skip header
  std::getline(iss, line);
  while (std::getline(iss, line))
  {
    if (!line.empty())
      ++rowCount;
  }
  if (rowCount != 5)
  {
    std::cerr << "Error: Output string does not contain 5 data rows. Found " << rowCount
              << std::endl;
    return EXIT_FAILURE;
  }
  else
  {
    std::cout << "Output string contains correct number of data rows." << std::endl;
  }

  // Check that the fourth data row contains expected values
  iss.clear();
  iss.seekg(0);
  std::getline(iss, line); // header
  // Skip three rows
  for (int i = 0; i < 3; ++i)
  {
    std::getline(iss, line);
  }
  if (std::getline(iss, line))
  {
    if (line.find("3,1.50,\"String 3\"") == std::string::npos)
    {
      std::cerr << "Error: Fourth data row does not match expected values." << std::endl;
      std::cerr << "\tExpected: 3,1.50,\"String 3\"\n";
      std::cerr << "\tFound: " << line << "\n";
      return EXIT_FAILURE;
    }
    else
    {
      std::cout << "Fourth data row matches expected values." << std::endl;
    }
  }

  // Now test scientific notation and increased precision
  intArray->Reset();
  doubleArray->Reset();
  stringArray->Reset();
  for (int i = 0; i < 5; ++i)
  {
    intArray->InsertNextValue(i * 200000);
    doubleArray->InsertNextValue(i * 0.00012345);
    stringArray->InsertNextValue("String " + vtk::to_string(i));
  }
  writer->SetPrecision(10);
  writer->SetNotationToScientific();
  writer->Write();

  output = writer->RegisterAndGetOutputString();
  outputStr = std::string(output);
  // Check that the header contains all column names
  if (outputStr.find("Integers") == std::string::npos ||
    outputStr.find("Doubles") == std::string::npos ||
    outputStr.find("Strings") == std::string::npos)
  {
    std::cerr << "Error: Output string missing column names." << std::endl;
  }
  // Check that there are 5 rows of data
  rowCount = 0;
  iss.str(outputStr);
  iss.clear();
  // Skip header
  std::getline(iss, line);
  while (std::getline(iss, line))
  {
    if (!line.empty())
      ++rowCount;
  }
  if (rowCount != 5)
  {
    std::cerr << "Error: Output string does not contain 5 data rows. Found " << rowCount
              << std::endl;
    return EXIT_FAILURE;
  }
  else
  {
    std::cout << "Output string contains correct number of data rows." << std::endl;
  }

  // Check that the fourth data row contains expected values
  iss.clear();
  iss.seekg(0);
  std::getline(iss, line); // header
  // Skip three rows
  for (int i = 0; i < 3; ++i)
  {
    std::getline(iss, line);
  }
  if (std::getline(iss, line))
  {
    if (line.find("600000,3.7035000000e-04,\"String 3\"") == std::string::npos)
    {
      std::cerr << "Error: Fourth data row does not match expected values." << std::endl;
      std::cerr << "\tExpected: 600000,3.7035000000e-04,\"String 3\"\n";
      std::cerr << "\tFound: " << line << "\n";
      return EXIT_FAILURE;
    }
    else
    {
      std::cout << "Fourth data row matches expected values." << std::endl;
    }
  }

  std::cout << "Delimited Text Output:\n" << output << std::endl;

  return EXIT_SUCCESS;
}
