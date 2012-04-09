/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFixedWidthTextReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include <vtkFixedWidthTextReader.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkVariant.h>
#include <vtkVariantArray.h>
#include <vtkTestUtilities.h>
#include <vtkIOStream.h>

int
TestFixedWidthTextReader(int argc, char *argv[])
{
  cout << "### Pass 1: No headers, field width 10, do not strip whitespace" << endl;

  vtkIdType i, j;
  char *filename = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                        "Data/fixedwidth.txt");

  cout << "Filename: " << filename << endl;

  vtkFixedWidthTextReader *reader = vtkFixedWidthTextReader::New();
  reader->SetHaveHeaders(false);
  reader->SetFieldWidth(10);
  reader->StripWhiteSpaceOff();
  reader->SetFileName(filename);
  reader->Update();

  cout << "Printing reader info..." << endl;
  reader->Print(cout);

  vtkTable *table = reader->GetOutput();

  cout << "FixedWidth text file has " << table->GetNumberOfRows()
       << " rows" << endl;
  cout << "FixedWidth text file has " << table->GetNumberOfColumns()
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
    }

  reader->Delete();
  delete [] filename;

  reader = vtkFixedWidthTextReader::New();
  filename = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                  "Data/fixedwidth.txt");

  reader->HaveHeadersOn();
  reader->SetFieldWidth(10);
  reader->StripWhiteSpaceOn();
  reader->SetFileName(filename);
  reader->Update();
  table = reader->GetOutput();


  cout << endl << "### Test 2: headers, field width 10, strip whitespace" << endl;

  cout << "Printing reader info..." << endl;
  reader->Print(cout);

  cout << "FixedWidth text file has " << table->GetNumberOfRows()
       << " rows" << endl;
  cout << "FixedWidth text file has " << table->GetNumberOfColumns()
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
    }

  reader->Delete();
  delete [] filename;

  return 0;
}



