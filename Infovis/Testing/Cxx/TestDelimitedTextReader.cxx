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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include <vtkDelimitedTextReader.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkVariant.h>
#include <vtkVariantArray.h>
#include <vtkTestUtilities.h>
#include <vtkIOStream.h>

int
TestDelimitedTextReader(int argc, char *argv[])
{
  vtkIdType i, j;
  char *filename = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                        "Data/delimited.txt");

  cout << "Filename: " << filename << endl;

  vtkDelimitedTextReader *reader = vtkDelimitedTextReader::New();
  reader->SetFieldDelimiter(':');
  reader->SetStringDelimiter('"');
  reader->SetUseStringDelimiter(true);
  reader->SetFileName(filename);
  reader->SetHaveHeaders(false);
  reader->Update();

  cout << "Printing reader info..." << endl;
  reader->Print(cout);

  vtkTable *table = reader->GetOutput();
 
  cout << "### Test 1: colon delimiter, no headers, do not merge consecutive delimiters" << endl;

  cout << "Delimited text file has " << table->GetNumberOfRows() 
       << " rows" << endl;
  cout << "Delimited text file has " << table->GetNumberOfColumns() 
       << " columns" << endl;
  cout << "Column names: " << endl;
  for (i = 0; i < table->GetNumberOfColumns(); ++i)
    {
    cout << "\tColumn " << i << ": " << table->GetColumn(i)->GetName() << endl;
    }

  cout << "Table contents:" << endl;
  
  for (i = 0; i < table->GetNumberOfRows(); ++i)
    {
    vtkVariantArray *row = table->GetRow(i);

    for (j = 0; j < row->GetNumberOfTuples(); ++j)
      {
      cout << "Row " << i << " column " << j << ": ";

      vtkVariant value = row->GetValue(j);
      if (! value.IsValid())
        {
        cout << "invalid value" << endl;
        }
      else
        {
        cout << "type " << value.GetTypeAsString() << " value " 
             << value.ToString() << endl;
        }
      }

    row->Delete();

    }
  
  reader->Delete();

  // Test 2: make sure the MergeConsecutiveDelimiters thing works
  reader = vtkDelimitedTextReader::New();
  filename = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                  "Data/delimited2.txt");

  reader->SetFieldDelimiter(',');
  reader->MergeConsecutiveDelimitersOn();
  reader->SetHaveHeaders(true);
  reader->SetFileName(filename);
  reader->Update();
  table = reader->GetOutput();

  cout << endl << "### Test 2: comma delimiter, headers, merge consecutive delimiters" << endl;

  cout << "Delimited text file has " << table->GetNumberOfRows() 
       << " rows" << endl;
  cout << "Delimited text file has " << table->GetNumberOfColumns() 
       << " columns" << endl;
  cout << "Column names: " << endl;
  for (i = 0; i < table->GetNumberOfColumns(); ++i)
    {
    cout << "\tColumn " << i << ": " << table->GetColumn(i)->GetName() << endl;
    }

  cout << "Table contents:" << endl;
  
  for (i = 0; i < table->GetNumberOfRows(); ++i)
    {
    vtkVariantArray *row = table->GetRow(i);

    for (j = 0; j < row->GetNumberOfTuples(); ++j)
      {
      cout << "Row " << i << " column " << j << ": ";

      vtkVariant value = row->GetValue(j);
      if (! value.IsValid())
        {
        cout << "invalid value" << endl;
        }
      else
        {
        cout << "type " << value.GetTypeAsString() << " value " 
             << value.ToString() << endl;
        }
      }

    row->Delete();

    }
  
  reader->Delete();

  return 0;
}

  

