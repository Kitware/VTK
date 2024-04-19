// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkSQLDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include "vtkDatabaseToTableReader.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkDatabaseToTableReader::vtkDatabaseToTableReader()
{
  this->Database = nullptr;
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkDatabaseToTableReader::~vtkDatabaseToTableReader() = default;

//------------------------------------------------------------------------------
bool vtkDatabaseToTableReader::SetDatabase(vtkSQLDatabase* db)
{
  if (!db)
  {
    return false;
  }
  this->Database = db;
  if (!this->Database->IsOpen())
  {
    vtkErrorMacro(<< "SetDatabase must be passed an open database connection");
    this->Database = nullptr;
    return false;
  }

  if (!this->TableName.empty())
  {
    return this->CheckIfTableExists();
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkDatabaseToTableReader::SetTableName(const char* name)
{
  std::string nameStr = name;
  this->TableName = nameStr;
  if (this->Database->IsOpen())
  {
    return this->CheckIfTableExists();
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkDatabaseToTableReader::CheckIfTableExists()
{
  if (!this->Database->IsOpen())
  {
    vtkErrorMacro(<< "CheckIfTableExists() called with no open database!");
    return false;
  }
  if (this->TableName.empty())
  {
    vtkErrorMacro(<< "CheckIfTableExists() called but no table name specified.");
    return false;
  }

  if (this->Database->GetTables()->LookupValue(this->TableName) == -1)
  {
    vtkErrorMacro(<< "Table " << this->TableName << " does not exist in the database!");
    this->TableName = "";
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkDatabaseToTableReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
