// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkTableToMySQLWriter and vtkMySQLToTableReader
// .SECTION Description
//

#include "vtkMySQLDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTableReader.h"
#include "vtkTableWriter.h"
#include "vtksys/SystemTools.hxx"

#include "vtkIOMySQLTestingCxxConfigure.h"
#include "vtkMySQLToTableReader.h"
#include "vtkTableToMySQLWriter.h"

#include <iostream>

int TestMySQLTableReadWrite(int argc, char* argv[])
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

  std::cerr << "opening a MySQL database connection" << std::endl;

  vtkMySQLDatabase* db =
    vtkMySQLDatabase::SafeDownCast(vtkSQLDatabase::CreateFromURL(VTK_MYSQL_TEST_URL));
  bool status = db->Open();

  if (!status)
  {
    std::cerr << "Couldn't open database.\n";
    return 1;
  }

  std::cerr << "creating a MySQL table from a vtkTable" << std::endl;
  vtkSmartPointer<vtkTableToMySQLWriter> writerToTest =
    vtkSmartPointer<vtkTableToMySQLWriter>::New();

  writerToTest->SetInputData(table);
  writerToTest->SetDatabase(db);
  writerToTest->SetTableName("tableTest");
  writerToTest->Update();

  std::cerr << "converting it back to a vtkTable" << std::endl;
  vtkSmartPointer<vtkMySQLToTableReader> readerToTest =
    vtkSmartPointer<vtkMySQLToTableReader>::New();

  readerToTest->SetDatabase(db);
  readerToTest->SetTableName("tableTest");
  readerToTest->Update();

  std::cerr << "writing the table out to disk" << std::endl;
  vtkSmartPointer<vtkTableWriter> tableFileWriter = vtkSmartPointer<vtkTableWriter>::New();
  tableFileWriter->SetFileName("TestMySQLTableReadWrite.vtk");
  tableFileWriter->SetInputConnection(readerToTest->GetOutputPort());
  tableFileWriter->Update();

  std::cerr << "verifying that it's the same as what we started with...";
  int result = 0;
  if (vtksys::SystemTools::FilesDiffer(argv[1], "TestMySQLTableReadWrite.vtk"))
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
  query->SetQuery("DROP TABLE tableTest");
  query->Execute();

  // clean up memory
  db->Delete();
  query->Delete();

  return result;
}
