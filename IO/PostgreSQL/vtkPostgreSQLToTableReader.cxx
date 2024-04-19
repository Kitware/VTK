// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPostgreSQLDatabase.h"
#include "vtkPostgreSQLQuery.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include "vtkPostgreSQLToTableReader.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPostgreSQLToTableReader);

//------------------------------------------------------------------------------
vtkPostgreSQLToTableReader::vtkPostgreSQLToTableReader() = default;

//------------------------------------------------------------------------------
vtkPostgreSQLToTableReader::~vtkPostgreSQLToTableReader() = default;

//------------------------------------------------------------------------------
int vtkPostgreSQLToTableReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  // Make sure we have all the information we need to provide a vtkTable
  if (!this->Database)
  {
    vtkErrorMacro(<< "No open database connection");
    return 1;
  }
  if (!this->Database->IsA("vtkPostgreSQLDatabase"))
  {
    vtkErrorMacro(<< "Wrong type of database for this reader");
    return 1;
  }
  if (this->TableName.empty())
  {
    vtkErrorMacro(<< "No table selected");
    return 1;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Return all data in the first piece ...
  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
  {
    return 1;
  }

  vtkTable* const output = vtkTable::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // perform a query to get the names and types of the columns
  std::string queryStr =
    "select column_name, data_type FROM information_schema.columns WHERE table_name = '";
  queryStr += this->TableName;
  queryStr += "';";
  vtkPostgreSQLQuery* query = static_cast<vtkPostgreSQLQuery*>(this->Database->GetQueryInstance());
  query->SetQuery(queryStr.c_str());
  if (!query->Execute())
  {
    vtkErrorMacro(<< "Error performing 'show columns' query");
  }

  // use the results of the query to create columns of the proper name & type
  std::vector<std::string> columnTypes;
  while (query->NextRow())
  {
    std::string columnName = query->DataValue(0).ToString();
    std::string columnType = query->DataValue(1).ToString();
    if ((columnType.find("int") != std::string::npos) ||
      (columnType.find("INT") != std::string::npos) ||
      (columnType.find("serial") != std::string::npos) ||
      (columnType.find("SERIAL") != std::string::npos))
    {
      vtkSmartPointer<vtkIntArray> column = vtkSmartPointer<vtkIntArray>::New();
      column->SetName(columnName.c_str());
      output->AddColumn(column);
      columnTypes.emplace_back("int");
    }
    else if ((columnType.find("double") != std::string::npos) ||
      (columnType.find("DOUBLE") != std::string::npos) ||
      (columnType.find("real") != std::string::npos) ||
      (columnType.find("REAL") != std::string::npos) ||
      (columnType.find("decimal") != std::string::npos) ||
      (columnType.find("DECIMAL") != std::string::npos) ||
      (columnType.find("numeric") != std::string::npos) ||
      (columnType.find("NUMERIC") != std::string::npos))
    {
      vtkSmartPointer<vtkDoubleArray> column = vtkSmartPointer<vtkDoubleArray>::New();
      column->SetName(columnName.c_str());
      output->AddColumn(column);
      columnTypes.emplace_back("double");
    }
    else
    {
      vtkSmartPointer<vtkStringArray> column = vtkSmartPointer<vtkStringArray>::New();
      column->SetName(columnName.c_str());
      output->AddColumn(column);
      columnTypes.emplace_back("string");
    }
  }

  // do a query to get the contents of the PostgreSQL table
  queryStr = "SELECT * FROM ";
  queryStr += this->TableName;
  query->SetQuery(queryStr.c_str());
  if (!query->Execute())
  {
    vtkErrorMacro(<< "Error performing 'select all' query");
  }

  // use the results of the query to populate the columns
  while (query->NextRow())
  {
    for (int col = 0; col < query->GetNumberOfFields(); ++col)
    {
      if (columnTypes[col] == "int")
      {
        vtkIntArray* column = static_cast<vtkIntArray*>(output->GetColumn(col));
        column->InsertNextValue(query->DataValue(col).ToInt());
      }
      else if (columnTypes[col] == "double")
      {
        vtkDoubleArray* column = static_cast<vtkDoubleArray*>(output->GetColumn(col));
        column->InsertNextValue(query->DataValue(col).ToDouble());
      }
      else
      {
        vtkStringArray* column = static_cast<vtkStringArray*>(output->GetColumn(col));
        column->InsertNextValue(query->DataValue(col).ToString());
      }
    }
  }

  query->Delete();
  return 1;
}

//------------------------------------------------------------------------------
void vtkPostgreSQLToTableReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
