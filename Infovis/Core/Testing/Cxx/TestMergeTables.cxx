/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMergeTables.cxx

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

#include <vtkDelimitedTextReader.h>
#include <vtkMergeTables.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkVariant.h>
#include <vtkVariantArray.h>
#include <vtkTestUtilities.h>
#include <vtkIOStream.h>

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int
TestMergeTables(int argc, char* argv[])
{
  char * filename1 = vtkTestUtilities::ExpandDataFileName(argc,argv,
                                                  "Data/Infovis/merge1.csv");
  char * filename2 = vtkTestUtilities::ExpandDataFileName(argc,argv,
                                                  "Data/Infovis/merge2.csv");

  VTK_CREATE(vtkDelimitedTextReader,reader1);
  reader1->SetFieldDelimiterCharacters(",");
  reader1->SetFileName(filename1);
  reader1->SetHaveHeaders(true);
  reader1->Update();

  vtkTable * table1 = reader1->GetOutput();

  VTK_CREATE(vtkDelimitedTextReader,reader2);
  reader2->SetFieldDelimiterCharacters(",");
  reader2->SetFileName(filename2);
  reader2->SetHaveHeaders(true);
  reader2->Update();

  vtkTable * table2 = reader2->GetOutput();

  cout << "Table 1:" << endl;
  table1->Dump(10);

  cout << "Table 2:" << endl;
  table2->Dump(10);

  VTK_CREATE(vtkMergeTables,merge);
  merge->SetInputData(0, table1);
  merge->SetInputData(1, table2);
  merge->SetMergeColumnsByName(true);
  merge->Update();

  vtkTable * mergedTable = merge->GetOutput();

  cout << "Merged Table:" << endl;
  mergedTable->Dump(10);

  // Test # of columns
  // - There should be 3: Col1, Col2, Col3
  if(mergedTable->GetNumberOfColumns() != 3)
    {
    cout << "ERROR: Wrong number of columns!" << endl
         << "       Expected 3, got " << mergedTable->GetNumberOfColumns()
         << endl;
    return 1;
    }

  delete [] filename1;
  delete [] filename2;

  return 0;
}
