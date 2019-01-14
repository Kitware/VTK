/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMySQLTableReadWrite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkTableToMySQLWriter and vtkMySQLToTableReader
// .SECTION Description
//

#include "vtksys/SystemTools.hxx"
#include "vtkSmartPointer.h"
#include "vtkMySQLDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkTable.h"
#include "vtkTableReader.h"
#include "vtkTableWriter.h"
#include "vtkToolkits.h"

#include "vtkTableToMySQLWriter.h"
#include "vtkMySQLToTableReader.h"
#include "vtkIOMySQLTestingCxxConfigure.h"

int TestMySQLTableReadWrite(int argc, char *argv[])
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

  cerr << "opening a MySQL database connection" << endl;

  vtkMySQLDatabase* db = vtkMySQLDatabase::SafeDownCast(
    vtkSQLDatabase::CreateFromURL( VTK_MYSQL_TEST_URL ) );
  bool status = db->Open();

  if ( ! status )
  {
    cerr << "Couldn't open database.\n";
    return 1;
  }

  cerr << "creating a MySQL table from a vtkTable" << endl;
  vtkSmartPointer<vtkTableToMySQLWriter> writerToTest =
    vtkSmartPointer<vtkTableToMySQLWriter>::New();

  writerToTest->SetInputData(table);
  writerToTest->SetDatabase(db);
  writerToTest->SetTableName("tableTest");
  writerToTest->Update();

  cerr << "converting it back to a vtkTable" << endl;
  vtkSmartPointer<vtkMySQLToTableReader> readerToTest =
    vtkSmartPointer<vtkMySQLToTableReader>::New();

  readerToTest->SetDatabase(db);
  readerToTest->SetTableName("tableTest");
  readerToTest->Update();

  cerr << "writing the table out to disk" << endl;
  vtkSmartPointer<vtkTableWriter> tableFileWriter =
    vtkSmartPointer<vtkTableWriter>::New();
  tableFileWriter->SetFileName("TestMySQLTableReadWrite.vtk");
  tableFileWriter->SetInputConnection(readerToTest->GetOutputPort());
  tableFileWriter->Update();

  cerr << "verifying that it's the same as what we started with...";
  int result = 0;
  if(vtksys::SystemTools::FilesDiffer(argv[1], "TestMySQLTableReadWrite.vtk"))
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
