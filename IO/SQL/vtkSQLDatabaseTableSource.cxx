/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLDatabaseTableSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkSQLDatabaseTableSource.h"

#include "vtkDataSetAttributes.h"
#include "vtkEventForwarderCommand.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkRowQueryToTable.h"
#include "vtkSQLDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

//---------------------------------------------------------------------------
class vtkSQLDatabaseTableSource::implementation
{
public:
  implementation() :
    Database(0),
    Query(0),
    Table(0)
  {
  }

  ~implementation()
  {
    if(this->Table)
      this->Table->Delete();

    if(this->Query)
      this->Query->Delete();

    if(this->Database)
      this->Database->Delete();
  }

  vtkStdString URL;
  vtkStdString Password;
  vtkStdString QueryString;

  vtkSQLDatabase* Database;
  vtkSQLQuery* Query;
  vtkRowQueryToTable* Table;
};

vtkStandardNewMacro(vtkSQLDatabaseTableSource);

//---------------------------------------------------------------------------
vtkSQLDatabaseTableSource::vtkSQLDatabaseTableSource() :
  Implementation(new implementation())
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->PedigreeIdArrayName = 0;
  this->SetPedigreeIdArrayName("id");
  this->GeneratePedigreeIds = true;

  // Set up eventforwarder
  this->EventForwarder = vtkEventForwarderCommand::New();
  this->EventForwarder->SetTarget(this);
}

//---------------------------------------------------------------------------
vtkSQLDatabaseTableSource::~vtkSQLDatabaseTableSource()
{
  delete this->Implementation;
  this->SetPedigreeIdArrayName(0);
  this->EventForwarder->Delete();
}

//---------------------------------------------------------------------------
void vtkSQLDatabaseTableSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "URL: " << this->Implementation->URL << endl;
  os << indent << "Query: " << this->Implementation->QueryString << endl;
  os << indent << "GeneratePedigreeIds: " << this->GeneratePedigreeIds << endl;
  os << indent << "PedigreeIdArrayName: " << this->PedigreeIdArrayName << endl;
}

vtkStdString vtkSQLDatabaseTableSource::GetURL()
{
  return this->Implementation->URL;
}

void vtkSQLDatabaseTableSource::SetURL(const vtkStdString& url)
{
  if(url == this->Implementation->URL)
    return;

  if(this->Implementation->Query)
    {
    this->Implementation->Query->Delete();
    this->Implementation->Query = 0;
    }

  if(this->Implementation->Database)
    {
    this->Implementation->Database->Delete();
    this->Implementation->Database = 0;
    }

  this->Implementation->URL = url;

  this->Modified();
}

void vtkSQLDatabaseTableSource::SetPassword(const vtkStdString& password)
{
  if(password == this->Implementation->Password)
    return;

  if(this->Implementation->Query)
    {
    this->Implementation->Query->Delete();
    this->Implementation->Query = 0;
    }

  if(this->Implementation->Database)
    {
    this->Implementation->Database->Delete();
    this->Implementation->Database = 0;
    }

  this->Implementation->Password = password;

  this->Modified();
}

vtkStdString vtkSQLDatabaseTableSource::GetQuery()
{
  return this->Implementation->QueryString;
}

void vtkSQLDatabaseTableSource::SetQuery(const vtkStdString& query)
{
  if(query == this->Implementation->QueryString)
    return;

  this->Implementation->QueryString = query;
  this->Modified();
}

//---------------------------------------------------------------------------
int vtkSQLDatabaseTableSource::RequestData(
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  if(this->Implementation->URL.empty())
    return 1;

  if(this->Implementation->QueryString.empty())
    return 1;

  if(!this->PedigreeIdArrayName)
    {
    vtkErrorMacro(<< "You must specify a pedigree id array name.");
    return 0;
    }

  if(!this->Implementation->Database)
    {
    this->Implementation->Database = vtkSQLDatabase::CreateFromURL(this->Implementation->URL);
    if(!this->Implementation->Database)
      {
      vtkErrorMacro(<< "Error creating database using URL: " << this->Implementation->URL.c_str());
      return 0;
      }

    if(!this->Implementation->Database->Open(this->Implementation->Password))
      {
      this->Implementation->Database->Delete();
      this->Implementation->Database = 0;

      vtkErrorMacro(<< "Error opening database: " << this->Implementation->URL.c_str());
      return 0;
      }
    }

  if(!this->Implementation->Query)
    {
    this->Implementation->Query = this->Implementation->Database->GetQueryInstance();
    if(!this->Implementation->Query)
      {
      vtkErrorMacro(<< "Internal error creating query instance.");
      return 0;
      }
    }

  // Set Progress Text
  this->SetProgressText("DatabaseTableSource");

  // I have a database: 5% progress
  this->UpdateProgress(.05);

  this->Implementation->Query->SetQuery(this->Implementation->QueryString.c_str());
  if(!this->Implementation->Query->Execute())
    {
    vtkErrorMacro(<< "Error executing query: " << this->Implementation->QueryString.c_str());
    return 0;
    }

  // Executed query: 33% progress
  this->UpdateProgress(.33);

  // Set Progress Text
  this->SetProgressText("DatabaseTableSource: RowQueryToTable");

  if(!this->Implementation->Table)
    {
    this->Implementation->Table = vtkRowQueryToTable::New();

    // Now forward progress events from the graph layout
    this->Implementation->Table->AddObserver(vtkCommand::ProgressEvent,
                                 this->EventForwarder);
    }
  this->Implementation->Table->SetQuery(this->Implementation->Query);
  this->Implementation->Table->Update();

  // Created Table: 66% progress
  this->SetProgressText("DatabaseTableSource");
  this->UpdateProgress(.66);

  vtkTable* const output = vtkTable::SafeDownCast(
    outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(this->Implementation->Table->GetOutput());

  if (this->GeneratePedigreeIds)
    {
    vtkSmartPointer<vtkIdTypeArray> pedigreeIds =
      vtkSmartPointer<vtkIdTypeArray>::New();
    vtkIdType numRows = output->GetNumberOfRows();
    pedigreeIds->SetNumberOfTuples(numRows);
    pedigreeIds->SetName(this->PedigreeIdArrayName);
    for (vtkIdType i = 0; i < numRows; ++i)
      {
      pedigreeIds->InsertValue(i, i);
      }
    output->GetRowData()->SetPedigreeIds(pedigreeIds);
    }
  else
    {
    vtkAbstractArray* arr =
      output->GetColumnByName(this->PedigreeIdArrayName);
    if (arr)
      {
      output->GetRowData()->SetPedigreeIds(arr);
      }
    else
      {
      vtkErrorMacro(<< "Could find pedigree id array: "
        << this->PedigreeIdArrayName);
      return 0;
      }
    }

  // Done: 100% progress
  this->UpdateProgress(1);

  return 1;
}

