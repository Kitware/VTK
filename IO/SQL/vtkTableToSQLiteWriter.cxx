/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToSQLiteWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSQLiteDatabase.h"
#include "vtkSQLiteQuery.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkVariant.h"

#include "vtkTableToSQLiteWriter.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTableToSQLiteWriter);

//----------------------------------------------------------------------------
vtkTableToSQLiteWriter::vtkTableToSQLiteWriter()
{
    this->Database = 0;
}

//----------------------------------------------------------------------------
vtkTableToSQLiteWriter::~vtkTableToSQLiteWriter()
{
}

//----------------------------------------------------------------------------
void vtkTableToSQLiteWriter::WriteData()
{
  //Make sure we have all the information we need to create an SQLite table
  if(!this->Database)
  {
    vtkErrorMacro(<<"No open database connection");
    return;
  }
  if(!this->Database->IsA("vtkSQLiteDatabase"))
  {
    vtkErrorMacro(<<"Wrong type of database for this writer");
    return;
  }
  if(this->TableName == "")
  {
    vtkErrorMacro(<<"No table name specified!");
    return;
  }

  //converting this table to SQLite will require two queries: one to create
  //the table, and another to populate its rows with data.
  std::string createTableQuery = "CREATE table ";
  createTableQuery += this->TableName;
  createTableQuery += "(";

  std::string insertPreamble = "INSERT into ";
  insertPreamble += this->TableName;
  insertPreamble += "(";

  //get the columns from the vtkTable to finish the query
  vtkIdType numColumns = this->GetInput()->GetNumberOfColumns();
  for(vtkIdType i = 0; i < numColumns; i++)
  {
    //get this column's name
    std::string columnName = this->GetInput()->GetColumn(i)->GetName();
    createTableQuery += columnName;
    insertPreamble += "'" + columnName + "'";

    //figure out what type of data is stored in this column
    std::string columnType = this->GetInput()->GetColumn(i)->GetClassName();

    if( (columnType.find("String") != std::string::npos) ||
        (columnType.find("Data") != std::string::npos) ||
        (columnType.find("Variant") != std::string::npos) )
    {
      createTableQuery += " TEXT";
    }
    else if( (columnType.find("Double") != std::string::npos) ||
             (columnType.find("Float") != std::string::npos) )
    {
      createTableQuery += " REAL";
    }
    else
    {
      createTableQuery += " INTEGER";
    }
    if(i == numColumns - 1)
    {
      createTableQuery += ");";
      insertPreamble += ") VALUES (";
    }
    else
    {
      createTableQuery += ", ";
      insertPreamble += ", ";
    }
  }

  //perform the create table query
  vtkSQLiteQuery *query =
    static_cast<vtkSQLiteQuery*>(this->Database->GetQueryInstance());

  query->SetQuery(createTableQuery.c_str());
  cout << "creating the table" << endl;
  if(!query->Execute())
  {
    vtkErrorMacro(<<"Error performing 'create table' query");
  }

  //iterate over the rows of the vtkTable to complete the insert query
  vtkIdType numRows = this->GetInput()->GetNumberOfRows();
  for(vtkIdType i = 0; i < numRows; i++)
  {
    std::string insertQuery = insertPreamble;
    for (vtkIdType j = 0; j < numColumns; j++)
    {
      insertQuery += "'" + this->GetInput()->GetValue(i, j).ToString() + "'";
      if(j < numColumns - 1)
      {
        insertQuery += ", ";
      }
    }
    insertQuery += ");";
    //perform the insert query for this row
    query->SetQuery(insertQuery.c_str());
    if(!query->Execute())
    {
      vtkErrorMacro(<<"Error performing 'insert' query");
    }
  }

  //cleanup and return
  query->Delete();
  return;
}

//----------------------------------------------------------------------------
int vtkTableToSQLiteWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//----------------------------------------------------------------------------
vtkTable* vtkTableToSQLiteWriter::GetInput()
{
  return vtkTable::SafeDownCast(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
vtkTable* vtkTableToSQLiteWriter::GetInput(int port)
{
  return vtkTable::SafeDownCast(this->Superclass::GetInput(port));
}

//----------------------------------------------------------------------------
void vtkTableToSQLiteWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
