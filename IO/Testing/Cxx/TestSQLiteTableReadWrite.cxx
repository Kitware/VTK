/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSQLiteTableReadWrite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkTableToSQLiteWriter and vtkSQLiteToTableReader
// .SECTION Description
//

#include "vtkSmartPointer.h"
#include "vtkSQLiteDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkTable.h"
#include "vtkTableReader.h"
#include "vtkTableWriter.h"

#include "vtkTableToSQLiteWriter.h"
#include "vtkSQLiteToTableReader.h"

#include "vtksys/SystemTools.hxx"

int TestSQLiteTableReadWrite(int argc, char *argv[])
{
  if ( argc <= 1 )
    {
    cerr << "Usage: " << argv[0] << " <.vtk table file>" << endl;
    return 1;
    }
  cerr << "reading a vtkTable from file" << endl;
  vtkSmartPointer<vtkTableReader> tableFileReader =
    vtkSmartPointer<vtkTableReader>::New();
  tableFileReader->SetFileName(argv[1]);
  vtkTable *table = tableFileReader->GetOutput();
  tableFileReader->Update();

  cerr << "opening an SQLite database connection" << endl;
  vtkSQLiteDatabase* db = vtkSQLiteDatabase::SafeDownCast(
    vtkSQLDatabase::CreateFromURL( "sqlite://local.db" ) );
  bool status = db->Open("", vtkSQLiteDatabase::CREATE_OR_CLEAR);
  if ( ! status )
    {
    cerr << "Couldn't open database using CREATE_OR_CLEAR.\n";
    return 1;
    }

  cerr << "creating an SQLite table from a vtkTable" << endl;
  vtkSmartPointer<vtkTableToSQLiteWriter> writerToTest =
    vtkSmartPointer<vtkTableToSQLiteWriter>::New();

  writerToTest->SetInput(table);
  writerToTest->SetDatabase(db);
  writerToTest->SetTableName("tableTest");
  writerToTest->Update();

  cerr << "converting it back to a vtkTable" << endl;
  vtkSmartPointer<vtkSQLiteToTableReader> readerToTest =
    vtkSmartPointer<vtkSQLiteToTableReader>::New();

  readerToTest->SetDatabase(db);
  readerToTest->SetTableName("tableTest");
  readerToTest->Update();

  cerr << "writing the table out to disk" << endl;
  vtkSmartPointer<vtkTableWriter> tableFileWriter =
    vtkSmartPointer<vtkTableWriter>::New();
  tableFileWriter->SetFileName("TestSQLiteTableReadWrite.vtk");
  tableFileWriter->SetInput(readerToTest->GetOutput());
  tableFileWriter->Update();

  cerr << "verifying that it's the same as what we started with...";
  int result = 0;
  if(vtksys::SystemTools::FilesDiffer(argv[1], "TestSQLiteTableReadWrite.vtk"))
    {
    cerr << "it's not." << endl;
    result = 1;
    }
  else
    {
    cerr << "it is!" << endl;
    }

  //drop the table we created
  vtkSQLQuery* query = db->GetQueryInstance();
  query->SetQuery("DROP TABLE tableTest");
  query->Execute();

  //clean up memory
  db->Delete();
  query->Delete();

  return result;
}
