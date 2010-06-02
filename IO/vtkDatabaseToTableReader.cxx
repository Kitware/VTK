/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDatabaseToTableReader.cxx

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

#include "vtkDatabaseToTableReader.h"

//----------------------------------------------------------------------------
vtkDatabaseToTableReader::vtkDatabaseToTableReader()
{
  this->Database = 0;
  vtkTable *output = vtkTable::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallelism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();
}

//----------------------------------------------------------------------------
vtkDatabaseToTableReader::~vtkDatabaseToTableReader()
{
}

//----------------------------------------------------------------------------
bool vtkDatabaseToTableReader::SetDatabase(vtkSQLDatabase *db)
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
    return this->CheckIfTableExists();
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkDatabaseToTableReader::SetTableName(const char *name)
{
  vtkstd::string nameStr = name;
  this->TableName = nameStr;
  if(this->Database->IsOpen())
    {
    return this->CheckIfTableExists();
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkDatabaseToTableReader::CheckIfTableExists()
{
  if(!this->Database->IsOpen())
    {
    vtkErrorMacro(<<"CheckIfTableExists() called with no open database!");
    return false;
    }
  if(this->TableName == "")
    {
    vtkErrorMacro(<<"CheckIfTableExists() called but no table name specified.");
    return false;
    }

  if(this->Database->GetTables()->LookupValue(this->TableName) == -1)
    {
    vtkErrorMacro(<<"Table " << this->TableName
                  << " does not exist in the database!");
    this->TableName = "";
    return false;
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkDatabaseToTableReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
