/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToDatabaseWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkTableToDatabaseWriter.h"

//----------------------------------------------------------------------------
vtkTableToDatabaseWriter::vtkTableToDatabaseWriter()
{
    this->Database = 0;
}

//----------------------------------------------------------------------------
vtkTableToDatabaseWriter::~vtkTableToDatabaseWriter()
{
}

//----------------------------------------------------------------------------
bool vtkTableToDatabaseWriter::SetDatabase(vtkSQLDatabase *db)
{
  if(!db)
    {
    return false;
    }
  this->Database = db;
  if(this->Database->IsOpen() == false)
    {
    vtkErrorMacro(<<"SetDatabase must be passed an open database connection");
    this->Database = 0;
    return false;
    }

  if(this->TableName != "")
    {
    return this->TableNameIsNew();
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkTableToDatabaseWriter::SetTableName(const char *name)
{
  std::string nameStr = name;
  this->TableName = nameStr;
  if(this->Database != 0)
    {
    return this->TableNameIsNew();
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkTableToDatabaseWriter::TableNameIsNew()
{
  if(this->Database == 0)
    {
    vtkErrorMacro(<<"TableNameIsNew() called with no open database!");
    return false;
    }

  if(this->TableName == "")
    {
    vtkErrorMacro(<<"TableNameIsNew() called but no table name specified.");
    return false;
    }

  vtkStringArray *tableNames = this->Database->GetTables();
  if(tableNames->LookupValue(this->TableName) == -1)
    {
    return true;
    }

  vtkErrorMacro(
    << "Table " << this->TableName << " already exists in the database.  "
    << "Please choose another name.");
  this->TableName = "";
  return false;
}

int vtkTableToDatabaseWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

vtkTable* vtkTableToDatabaseWriter::GetInput()
{
  return vtkTable::SafeDownCast(this->Superclass::GetInput());
}

vtkTable* vtkTableToDatabaseWriter::GetInput(int port)
{
  return vtkTable::SafeDownCast(this->Superclass::GetInput(port));
}

//----------------------------------------------------------------------------
void vtkTableToDatabaseWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
