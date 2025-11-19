// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkTable.h>
#include <vtkTemporalDelimitedTextReader.h>
#include <vtkTestUtilities.h>

#include <iostream>

int TestTemporalDelimitedTextReader(int argc, char* argv[])
{
  const char* testFNames = "Data/vehicle_data.csv";
  const char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, testFNames);
  std::string filename = fname;
  delete[] fname;

  { // TEST 1: No TimeStepColumn given, output the whole CSV
    vtkNew<vtkTemporalDelimitedTextReader> reader;

    reader->SetFileName(filename.c_str());
    reader->UpdateInformation();
    // Nothing should be done when no FieldDelimiterCharacters have been set

    vtkTable* emptyTable = reader->GetOutput();
    if (emptyTable != nullptr && emptyTable->GetNumberOfRows() != 0)
    {
      std::cout << "ERROR in test 1: output should be empty (no FieldDelimiterCharacters given)"
                << std::endl;
      if (emptyTable != nullptr)
      {
        std::cout << "The output has " << emptyTable->GetNumberOfRows() << " rows " << std::endl;
      }
      std::cout << "Printing reader info..." << std::endl;
      reader->Print(std::cout);
    }

    reader->SetHaveHeaders(true);
    reader->SetFieldDelimiterCharacters(",");
    reader->Update();

    vtkTable* table = reader->GetOutput();
    if (table->GetNumberOfColumns() != 7)
    {
      std::cout << "ERROR in test 1: Wrong number of columns:" << std::endl;
      std::cout << table->GetNumberOfColumns() << " should be 7." << std::endl;
      std::cout << "Printing reader info..." << std::endl;
      reader->Print(std::cout);
      return 1;
    }
    if (table->GetNumberOfRows() != 392)
    {
      std::cout << "ERROR in test 1: Wrong number of rows:" << std::endl;
      std::cout << table->GetNumberOfRows() << " should be 392." << std::endl;
      std::cout << "Printing reader info..." << std::endl;
      reader->Print(std::cout);
      return 1;
    }
  }

  { // TEST 2: A specific Timestep
    vtkNew<vtkTemporalDelimitedTextReader> reader;

    reader->SetFileName(filename.c_str());
    reader->SetFieldDelimiterCharacters(",");
    reader->SetHaveHeaders(true);
    reader->SetTimeColumnName("Year");
    reader->UpdateTimeStep(71);

    vtkTable* table = reader->GetOutput();
    if (table->GetNumberOfColumns() != 6)
    {
      // The time column have been removed
      std::cout << "ERROR in test 2: Wrong number of columns:";
      std::cout << table->GetNumberOfColumns() << " should be 6." << std::endl;
      std::cout << "Printing reader info..." << std::endl;
      reader->Print(std::cout);
      return 1;
    }
    if (table->GetNumberOfRows() != 27)
    {
      // 27 entry for the year 71
      std::cout << "ERROR in test 2: Wrong number of rows:";
      std::cout << table->GetNumberOfRows() << " should be 27 at timestep 71." << std::endl;
      std::cout << "Printing reader info..." << std::endl;
      reader->Print(std::cout);
      return 1;
    }

    reader->SetRemoveTimeStepColumn(false);
    reader->Update();

    vtkTable* table2 = reader->GetOutput();
    if (table2->GetNumberOfColumns() != 7)
    {
      // The time column is back again
      std::cout << "ERROR in test 3: Wrong number of columns:" << std::endl;
      std::cout << table->GetNumberOfColumns() << " should be 7." << std::endl;
      std::cout << "Printing reader info..." << std::endl;
      reader->Print(std::cout);
      return 1;
    }
    if (table2->GetNumberOfRows() != 27)
    {
      // 27 entry for the year 71
      std::cout << "ERROR in test 3: Wrong number of rows:" << std::endl;
      std::cout << table->GetNumberOfRows() << " should be 27 at timestep 71." << std::endl;
      std::cout << "Printing reader info..." << std::endl;
      reader->Print(std::cout);
      return 1;
    }
  }

  return 0;
}
