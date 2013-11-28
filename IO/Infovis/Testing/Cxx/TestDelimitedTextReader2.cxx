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

#include <vtkDelimitedTextReader.h>
#include <vtkTable.h>
#include <vtkTestUtilities.h>

// This test mainly tests the capability of the DelimitedTextReader accepting
//  both a file and a text string as the input
int TestDelimitedTextReader2(int argc, char *argv[])
{
  //------------  test the reader with an input file-----------------
  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/authors.csv");

  vtkDelimitedTextReader *reader = vtkDelimitedTextReader::New();
  reader->SetFileName(filename);
  reader->SetHaveHeaders(1);
  reader->SetDetectNumericColumns(1);
  reader->Update();
  delete[] filename;

  vtkTable* table = reader->GetOutput();
  table->Dump();
  cout << "Printing reader info..." << endl;
  reader->Print(cout);

  if (table->GetNumberOfRows() != 6)
    {
    cout << "ERROR: Wrong number of rows: " << table->GetNumberOfRows()<<endl;
    return 1;
    }
  if (table->GetNumberOfColumns() != 6)
    {
    cout << "ERROR: Wrong number of columns: " << table->GetNumberOfColumns()<<endl;
    return 1;
    }

  reader->Delete();


  //------------  test the reader with an input string-----------------
  char inputString[] = ",awesomeness,fitness,region\r\nAbby,1,2,china\r\nBob,5,0.2,US\r\nCatie,3,0.3,UK\r\nDavid,2,100,UK\r\nGrace,4,20,US\r\nIlknur,6,5,Turkey\r\n";

  vtkDelimitedTextReader *reader2 = vtkDelimitedTextReader::New();
  reader2->SetHaveHeaders(1);
  reader2->SetReadFromInputString(1);
  reader2->SetInputString(inputString);
  reader2->SetDetectNumericColumns(1);
  reader2->Update();

  vtkTable* table2 = reader2->GetOutput();
  table2->Dump();
  cout << "Printing reader2 info..." << endl;
  reader2->Print(cout);

  if (table2->GetNumberOfRows() != 6)
    {
    cout << "ERROR: Wrong number of rows: " << table2->GetNumberOfRows()<<endl;
    return 1;
    }
  if (table2->GetNumberOfColumns() != 4)
    {
    cout << "ERROR: Wrong number of columns: " << table2->GetNumberOfColumns()<<endl;
    return 1;
    }

  reader2->Delete();

  return 0;
}
