/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPostgreSQLTableReadWrite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkTableToPostgreSQLWriter and vtkPostgreSQLToTableReader
// .SECTION Description
//

#include "vtksys/SystemTools.hxx"
#include "vtkSmartPointer.h"
#include "vtkPostgreSQLDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkTable.h"
#include "vtkTableReader.h"
#include "vtkTableWriter.h"
#include "vtkToolkits.h"

#include "vtkTableToPostgreSQLWriter.h"
#include "vtkPostgreSQLToTableReader.h"

int TestPostgreSQLTableReadWrite(int argc, char *argv[])
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

  cerr << "opening a PostgreSQL database connection" << endl;

  vtkPostgreSQLDatabase* db = vtkPostgreSQLDatabase::SafeDownCast(
    vtkSQLDatabase::CreateFromURL( VTK_PSQL_TEST_URL ) );
  bool status = db->Open();

  if ( ! status )
    {
    cerr << "Couldn't open database.\n";
    return 1;
    }

  cerr << "creating a PostgreSQL table from a vtkTable" << endl;
  vtkSmartPointer<vtkTableToPostgreSQLWriter> writerToTest =
    vtkSmartPointer<vtkTableToPostgreSQLWriter>::New();

  writerToTest->SetInput(table);
  writerToTest->SetDatabase(db);
  writerToTest->SetTableName("tabletest");
  writerToTest->Update();

  cerr << "converting it back to a vtkTable" << endl;
  vtkSmartPointer<vtkPostgreSQLToTableReader> readerToTest =
    vtkSmartPointer<vtkPostgreSQLToTableReader>::New();

  readerToTest->SetDatabase(db);
  readerToTest->SetTableName("tabletest");
  readerToTest->Update();

  cerr << "writing the table out to disk" << endl;
  vtkSmartPointer<vtkTableWriter> tableFileWriter =
    vtkSmartPointer<vtkTableWriter>::New();
  tableFileWriter->SetFileName("TestPostgreSQLTableReadWrite.vtk");
  tableFileWriter->SetInput(readerToTest->GetOutput());
  tableFileWriter->Update();

  cerr << "verifying that it's the same as what we started with...";
  int result = 0;
  if(vtksys::SystemTools::FilesDiffer(argv[1], "TestPostgreSQLTableReadWrite.vtk"))
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
  query->SetQuery("DROP TABLE tabletest");
  query->Execute();

  //clean up memory
  db->Delete();
  query->Delete();

  return result;
}
