// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkTableToPostgreSQLWriter and vtkPostgreSQLToTableReader
// .SECTION Description
//

#include "vtkPostgreSQLDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTableReader.h"
#include "vtkTableWriter.h"
#include "vtksys/SystemTools.hxx"

#include "vtkIOPostgresSQLTestingCxxConfigure.h"
#include "vtkPostgreSQLToTableReader.h"
#include "vtkTableToPostgreSQLWriter.h"

#include <iostream>

int TestPostgreSQLTableReadWrite(int argc, char* argv[])
{
  if (argc <= 1)
  {
    std::cerr << "Usage: " << argv[0] << " <.vtk table file>" << std::endl;
    return 1;
  }
  std::cerr << "reading a vtkTable from file" << std::endl;
  vtkSmartPointer<vtkTableReader> tableFileReader = vtkSmartPointer<vtkTableReader>::New();
  tableFileReader->SetFileName(argv[1]);
  vtkTable* table = tableFileReader->GetOutput();
  tableFileReader->Update();

  std::cerr << "opening a PostgreSQL database connection" << std::endl;

  vtkPostgreSQLDatabase* db =
    vtkPostgreSQLDatabase::SafeDownCast(vtkSQLDatabase::CreateFromURL(VTK_PSQL_TEST_URL));
  std::string realDatabase = db->GetDatabaseName();
  db->SetDatabaseName("template1"); // This is guaranteed to exist
  bool status = db->Open();
  if (!status)
  {
    std::cerr << "Couldn't open database.\n";
    return 1;
  }

  if (!db->CreateDatabase(realDatabase.c_str(), true))
  {
    std::cerr << "Error: " << db->GetLastErrorText() << std::endl;
  }
  db->SetDatabaseName(realDatabase.c_str());
  if (!db->Open())
  {
    std::cerr << "Error: " << db->GetLastErrorText() << std::endl;
    return 1;
  }

  std::cerr << "creating a PostgreSQL table from a vtkTable" << std::endl;
  vtkSmartPointer<vtkTableToPostgreSQLWriter> writerToTest =
    vtkSmartPointer<vtkTableToPostgreSQLWriter>::New();

  writerToTest->SetInputData(table);
  writerToTest->SetDatabase(db);
  writerToTest->SetTableName("tabletest");
  writerToTest->Update();

  std::cerr << "converting it back to a vtkTable" << std::endl;
  vtkSmartPointer<vtkPostgreSQLToTableReader> readerToTest =
    vtkSmartPointer<vtkPostgreSQLToTableReader>::New();

  readerToTest->SetDatabase(db);
  readerToTest->SetTableName("tabletest");
  readerToTest->Update();

  std::cerr << "writing the table out to disk" << std::endl;
  vtkSmartPointer<vtkTableWriter> tableFileWriter = vtkSmartPointer<vtkTableWriter>::New();
  tableFileWriter->SetFileName("TestPostgreSQLTableReadWrite.vtk");
  tableFileWriter->SetInputConnection(readerToTest->GetOutputPort());
  tableFileWriter->Update();

  std::cerr << "verifying that it's the same as what we started with...";
  int result = 0;
  if (vtksys::SystemTools::FilesDiffer(argv[1], "TestPostgreSQLTableReadWrite.vtk"))
  {
    std::cerr << "it's not." << std::endl;
    result = 1;
  }
  else
  {
    std::cerr << "it is!" << std::endl;
  }

  // drop the table we created
  vtkSQLQuery* query = db->GetQueryInstance();
  query->SetQuery("DROP TABLE tabletest");
  query->Execute();

  std::cerr << "dropping the database...";

  if (!db->DropDatabase(realDatabase.c_str()))
  {
    std::cout << "Drop of \"" << realDatabase << "\" failed.\n";
    std::cerr << "\"" << db->GetLastErrorText() << "\"" << std::endl;
  }

  // clean up memory
  db->Delete();
  query->Delete();

  return result;
}
