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
#include <vtkNew.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkVariant.h>
#include <vtkVariantArray.h>
#include <vtkTestUtilities.h>
#include <vtkTestErrorObserver.h>
#include <vtkIOStream.h>

#define CHECK_ERROR_MSG(observer, msg)   \
  { \
  std::string expectedMsg(msg); \
  if (!observer->GetError()) \
  { \
    std::cout << "ERROR: Failed to catch any error. Expected the error message to contain \"" << expectedMsg << std::endl; \
  } \
  else \
  { \
    std::string gotMsg(observer->GetErrorMessage()); \
    if (gotMsg.find(expectedMsg) == std::string::npos) \
    { \
      std::cout << "ERROR: Error message does not contain \"" << expectedMsg << "\" got \n\"" << gotMsg << std::endl; \
    } \
  } \
  } \
  observer->Clear()

int TestFixedWidthTextReader(int argc, char *argv[])
{
  std::cout << "### Pass 1: No headers, field width 10, do not strip whitespace" << std::endl;

  vtkIdType i, j;
  char *filename = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                        "Data/fixedwidth.txt");

  std::cout << "Filename: " << filename << std::endl;

  vtkNew<vtkTest::ErrorObserver> errorObserver1;

  vtkFixedWidthTextReader *reader = vtkFixedWidthTextReader::New();
  reader->SetHaveHeaders(false);
  reader->SetFieldWidth(10);
  reader->StripWhiteSpaceOff();
  reader->SetFileName(filename);
  reader->SetTableErrorObserver(errorObserver1.GetPointer());
  reader->Update();
  CHECK_ERROR_MSG(errorObserver1,
                  "Incorrect number of tuples in SetRow. Expected 4, but got 6");
  std::cout << "Printing reader info..." << std::endl;
  reader->Print(std::cout);

  vtkTable *table = reader->GetOutput();

  std::cout << "FixedWidth text file has " << table->GetNumberOfRows()
            << " rows" << std::endl;
  std::cout << "FixedWidth text file has " << table->GetNumberOfColumns()
            << " columns" << std::endl;
  std::cout << "Column names: " << std::endl;

  for (i = 0; i < table->GetNumberOfColumns(); ++i)
  {
    std::cout << "\tColumn " << i << ": "
              << table->GetColumn(i)->GetName() << std::endl;
  }

  std::cout << "Table contents:" << std::endl;

  for (i = 0; i < table->GetNumberOfRows(); ++i)
  {
    vtkVariantArray *row = table->GetRow(i);

    for (j = 0; j < row->GetNumberOfTuples(); ++j)
    {
      std::cout << "Row " << i << " column " << j << ": ";

      vtkVariant value = row->GetValue(j);
      if (! value.IsValid())
      {
        std::cout << "invalid value" << std::endl;
      }
      else
      {
        std::cout << "type " << value.GetTypeAsString() << " value "
             << value.ToString() << std::endl;
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
  reader->SetTableErrorObserver(errorObserver1.GetPointer());
  reader->Update();
  CHECK_ERROR_MSG(errorObserver1,
                  "Incorrect number of tuples in SetRow. Expected 4, but got 6");

  table = reader->GetOutput();


  std::cout << std::endl << "### Test 2: headers, field width 10, strip whitespace" << std::endl;

  std::cout << "Printing reader info..." << std::endl;
  reader->Print(std::cout);

  std::cout << "FixedWidth text file has " << table->GetNumberOfRows()
       << " rows" << std::endl;
  std::cout << "FixedWidth text file has " << table->GetNumberOfColumns()
       << " columns" << std::endl;
  std::cout << "Column names: " << std::endl;
  for (i = 0; i < table->GetNumberOfColumns(); ++i)
  {
    std::cout << "\tColumn " << i << ": " << table->GetColumn(i)->GetName() << std::endl;
  }

  std::cout << "Table contents:" << std::endl;

  for (i = 0; i < table->GetNumberOfRows(); ++i)
  {
    vtkVariantArray *row = table->GetRow(i);

    for (j = 0; j < row->GetNumberOfTuples(); ++j)
    {
      std::cout << "Row " << i << " column " << j << ": ";

      vtkVariant value = row->GetValue(j);
      if (! value.IsValid())
      {
        std::cout << "invalid value" << std::endl;
      }
      else
      {
        std::cout << "type " << value.GetTypeAsString() << " value "
             << value.ToString() << std::endl;
      }
    }
  }

  reader->Delete();
  delete [] filename;

  return 0;
}



