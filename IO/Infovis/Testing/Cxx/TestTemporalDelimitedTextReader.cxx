/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDelimitedTextReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkTable.h>
#include <vtkTemporalDelimitedTextReader.h>
#include <vtkTestUtilities.h>

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
      cout << "ERROR in test 1: output should be empty (no FieldDelimiterCharacters given)" << endl;
      if (emptyTable != nullptr)
      {
        cout << "The output has " << emptyTable->GetNumberOfRows() << " rows " << endl;
      }
      cout << "Printing reader info..." << endl;
      reader->Print(cout);
    }

    reader->SetHaveHeaders(true);
    reader->SetFieldDelimiterCharacters(",");
    reader->Update();

    vtkTable* table = reader->GetOutput();
    if (table->GetNumberOfColumns() != 7)
    {
      cout << "ERROR in test 1: Wrong number of columns:" << endl;
      cout << table->GetNumberOfColumns() << " sould be 7." << endl;
      cout << "Printing reader info..." << endl;
      reader->Print(cout);
      return 1;
    }
    if (table->GetNumberOfRows() != 392)
    {
      cout << "ERROR in test 1: Wrong number of rows:" << endl;
      cout << table->GetNumberOfRows() << " sould be 392." << endl;
      cout << "Printing reader info..." << endl;
      reader->Print(cout);
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
      cout << "ERROR in test 2: Wrong number of columns:";
      cout << table->GetNumberOfColumns() << " sould be 6." << endl;
      cout << "Printing reader info..." << endl;
      reader->Print(cout);
      return 1;
    }
    if (table->GetNumberOfRows() != 27)
    {
      // 27 entry for the year 71
      cout << "ERROR in test 2: Wrong number of rows:";
      cout << table->GetNumberOfRows() << " should be 27 at timestep 71." << endl;
      cout << "Printing reader info..." << endl;
      reader->Print(cout);
      return 1;
    }

    reader->SetRemoveTimeStepColumn(false);
    reader->Update();

    vtkTable* table2 = reader->GetOutput();
    if (table2->GetNumberOfColumns() != 7)
    {
      // The time column is back again
      cout << "ERROR in test 3: Wrong number of columns:" << endl;
      cout << table->GetNumberOfColumns() << " sould be 7." << endl;
      cout << "Printing reader info..." << endl;
      reader->Print(cout);
      return 1;
    }
    if (table2->GetNumberOfRows() != 27)
    {
      // 27 entry for the year 71
      cout << "ERROR in test 3: Wrong number of rows:" << endl;
      cout << table->GetNumberOfRows() << " should be 27 at timestep 71." << endl;
      cout << "Printing reader info..." << endl;
      reader->Print(cout);
      return 1;
    }
  }

  return 0;
}
