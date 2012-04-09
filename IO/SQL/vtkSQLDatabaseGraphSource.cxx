/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLDatabaseGraphSource.cxx

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

#include "vtkSQLDatabaseGraphSource.h"

#include "vtkDataSetAttributes.h"
#include "vtkDirectedGraph.h"
#include "vtkUndirectedGraph.h"
#include "vtkEventForwarderCommand.h"
#include "vtkExecutive.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkRowQueryToTable.h"
#include "vtkSQLDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkSmartPointer.h"
#include "vtkTableToGraph.h"
#include "vtkUndirectedGraph.h"

//---------------------------------------------------------------------------
class vtkSQLDatabaseGraphSource::implementation
{
public:
  implementation() :
    Database(0),
    EdgeQuery(0),
    EdgeTable(0),
    VertexQuery(0),
    VertexTable(0),
    TableToGraph(vtkTableToGraph::New())
  {
  }

  ~implementation()
  {
    if(this->TableToGraph)
      this->TableToGraph->Delete();
    if(this->VertexTable)
      this->VertexTable->Delete();
    if(this->VertexQuery)
      this->VertexQuery->Delete();
    if(this->EdgeTable)
      this->EdgeTable->Delete();
    if(this->EdgeQuery)
      this->EdgeQuery->Delete();
    if(this->Database)
      this->Database->Delete();
  }

  vtkStdString URL;
  vtkStdString Password;
  vtkStdString EdgeQueryString;
  vtkStdString VertexQueryString;

  vtkSQLDatabase* Database;
  vtkSQLQuery* EdgeQuery;
  vtkRowQueryToTable* EdgeTable;
  vtkSQLQuery* VertexQuery;
  vtkRowQueryToTable* VertexTable;
  vtkTableToGraph* TableToGraph;
};

vtkStandardNewMacro(vtkSQLDatabaseGraphSource);

//---------------------------------------------------------------------------
vtkSQLDatabaseGraphSource::vtkSQLDatabaseGraphSource() :
  Implementation(new implementation()),
  Directed(true)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->GenerateEdgePedigreeIds = true;
  this->EdgePedigreeIdArrayName = 0;
  this->SetEdgePedigreeIdArrayName("id");
  
  // Set up eventforwarder
  this->EventForwarder = vtkEventForwarderCommand::New();
  this->EventForwarder->SetTarget(this);
  
  // Now forward progress events from the graph layout
  this->Implementation->TableToGraph->AddObserver(vtkCommand::ProgressEvent, 
                                 this->EventForwarder);
}

//---------------------------------------------------------------------------
vtkSQLDatabaseGraphSource::~vtkSQLDatabaseGraphSource()
{
  delete this->Implementation;
  this->SetEdgePedigreeIdArrayName(0);
  this->EventForwarder->Delete();
}

//---------------------------------------------------------------------------
void vtkSQLDatabaseGraphSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "URL: " << this->Implementation->URL << endl;
  os << indent << "EdgeQuery: " << this->Implementation->EdgeQueryString << endl;
  os << indent << "VertexQuery: " << this->Implementation->VertexQueryString << endl;
  os << indent << "Directed: " << this->Directed << endl;
  os << indent << "GenerateEdgePedigreeIds: " << this->GenerateEdgePedigreeIds << endl;
  os << indent << "EdgePedigreeIdArrayName: " << (this->EdgePedigreeIdArrayName ? this->EdgePedigreeIdArrayName : "(null)" ) << endl;
}

vtkStdString vtkSQLDatabaseGraphSource::GetURL()
{
  return this->Implementation->URL;
}

void vtkSQLDatabaseGraphSource::SetURL(const vtkStdString& url)
{
  if(url == this->Implementation->URL)
    return;

  if(this->Implementation->EdgeQuery)
    {
    this->Implementation->EdgeQuery->Delete();
    this->Implementation->EdgeQuery = 0;
    }
  
  if(this->Implementation->VertexQuery)
    {
    this->Implementation->VertexQuery->Delete();
    this->Implementation->VertexQuery = 0;
    }
  
  if(this->Implementation->Database)
    {
    this->Implementation->Database->Delete();
    this->Implementation->Database = 0;
    }
  
  this->Implementation->URL = url;
  
  this->Modified();
}

void vtkSQLDatabaseGraphSource::SetPassword(const vtkStdString& password)
{
  if(password == this->Implementation->Password)
    return;

  if(this->Implementation->EdgeQuery)
    {
    this->Implementation->EdgeQuery->Delete();
    this->Implementation->EdgeQuery = 0;
    }
  
  if(this->Implementation->VertexQuery)
    {
    this->Implementation->VertexQuery->Delete();
    this->Implementation->VertexQuery = 0;
    }
  
  if(this->Implementation->Database)
    {
    this->Implementation->Database->Delete();
    this->Implementation->Database = 0;
    }
  
  this->Implementation->Password = password;
  
  this->Modified();
}

vtkStdString vtkSQLDatabaseGraphSource::GetEdgeQuery()
{
  return this->Implementation->EdgeQueryString;
}

void vtkSQLDatabaseGraphSource::SetEdgeQuery(const vtkStdString& query)
{
  if(query == this->Implementation->EdgeQueryString)
    return;

  this->Implementation->EdgeQueryString = query;
  this->Modified();
}

vtkStdString vtkSQLDatabaseGraphSource::GetVertexQuery()
{
  return this->Implementation->VertexQueryString;
}

void vtkSQLDatabaseGraphSource::SetVertexQuery(const vtkStdString& query)
{
  if(query == this->Implementation->VertexQueryString)
    return;

  this->Implementation->VertexQueryString = query;
  this->Modified();
}

void vtkSQLDatabaseGraphSource::AddLinkVertex(const char* column, const char* domain, int hidden)
{
  this->Implementation->TableToGraph->AddLinkVertex(column, domain, hidden);
  this->Modified();
}

void vtkSQLDatabaseGraphSource::ClearLinkVertices()
{
  this->Implementation->TableToGraph->ClearLinkVertices();
  this->Modified();
}

void vtkSQLDatabaseGraphSource::AddLinkEdge(const char* column1, const char* column2)
{
  this->Implementation->TableToGraph->AddLinkEdge(column1, column2);
  this->Modified();
}

void vtkSQLDatabaseGraphSource::ClearLinkEdges()
{
  this->Implementation->TableToGraph->ClearLinkEdges();
  this->Modified();
}


//---------------------------------------------------------------------------
int vtkSQLDatabaseGraphSource::RequestDataObject(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector*)
{
  vtkGraph* output = 0;
  if(this->Directed)
    {
    output = vtkDirectedGraph::New();
    }
  else
    {
    output = vtkUndirectedGraph::New();
    }
  this->GetExecutive()->SetOutputData(0, output);
  output->Delete();

  return 1;
}

//---------------------------------------------------------------------------
int vtkSQLDatabaseGraphSource::RequestData(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector* outputVector)
{
  if(this->Implementation->URL.empty())
    return 1;

  if(this->Implementation->EdgeQueryString.empty())
    return 1;
    
  // Set Progress Text
  this->SetProgressText("DatabaseGraphSource");  
    
  // I've started so 1% progress :) 
  this->UpdateProgress(.01);
  
  // Setup the database if it doesn't already exist ...
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
    
  // I have a database: 5% progress 
  this->UpdateProgress(.05);

  // Setup the edge query if it doesn't already exist ...
  if(!this->Implementation->EdgeQuery)
    {
    this->Implementation->EdgeQuery = this->Implementation->Database->GetQueryInstance();
    if(!this->Implementation->EdgeQuery)
      {
      vtkErrorMacro(<< "Internal error creating edge query instance.");
      return 0;
      }
    }

  this->Implementation->EdgeQuery->SetQuery(this->Implementation->EdgeQueryString.c_str());
  if(!this->Implementation->EdgeQuery->Execute())
    {
    vtkErrorMacro(<< "Error executing edge query: " << this->Implementation->EdgeQueryString.c_str());
    return 0;
    }
    
  // Executed edge query: 30% progress
  this->UpdateProgress(.3);   

  if(!this->Implementation->EdgeTable)
    {
    this->Implementation->EdgeTable = vtkRowQueryToTable::New();
    }
  this->Implementation->EdgeTable->SetQuery(this->Implementation->EdgeQuery);

  this->Implementation->TableToGraph->SetInputConnection(0, this->Implementation->EdgeTable->GetOutputPort());

  // Setup the (optional) vertex query if it doesn't already exist ...
  if(this->Implementation->VertexQueryString.size())
    {
    if(!this->Implementation->VertexQuery)
      {
      this->Implementation->VertexQuery = this->Implementation->Database->GetQueryInstance();
      if(!this->Implementation->VertexQuery)
        {
        vtkErrorMacro(<< "Internal error creating vertex query instance.");
        return 0;
        }
      }

    this->Implementation->VertexQuery->SetQuery(this->Implementation->VertexQueryString.c_str());
    if(!this->Implementation->VertexQuery->Execute())
      {
      vtkErrorMacro(<< "Error executing vertex query: " << this->Implementation->VertexQueryString.c_str());
      return 0;
      }
      
    // Executed vertex query: 50% progress 
    this->UpdateProgress(.5); 

    if(!this->Implementation->VertexTable)
      {
      this->Implementation->VertexTable = vtkRowQueryToTable::New();
      
      }
    this->Implementation->VertexTable->SetQuery(this->Implementation->VertexQuery);

    this->Implementation->TableToGraph->SetInputConnection(1, this->Implementation->VertexTable->GetOutputPort());
    }
    
  // Set Progress Text
  this->SetProgressText("DatabaseGraphSource:TableToGraph");

  // Get the graph output ...
  this->Implementation->TableToGraph->SetDirected(this->Directed);
  this->Implementation->TableToGraph->Update();
  
  // Set Progress Text
  this->SetProgressText("DatabaseGraphSource");
  
  // Finished table to graph: 90% progress 
  this->UpdateProgress(.9); 

  vtkGraph* const output = vtkGraph::SafeDownCast(
    outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(this->Implementation->TableToGraph->GetOutput());

  if (this->GenerateEdgePedigreeIds)
    {
    vtkIdType numEdges = output->GetNumberOfEdges();
    vtkSmartPointer<vtkIdTypeArray> arr =
      vtkSmartPointer<vtkIdTypeArray>::New();
    arr->SetName(this->EdgePedigreeIdArrayName);
    arr->SetNumberOfTuples(numEdges);
    for (vtkIdType i = 0; i < numEdges; ++i)
      {
      arr->InsertValue(i, i);
      }
    output->GetEdgeData()->SetPedigreeIds(arr);
    }
  else
    {
    vtkAbstractArray* arr = output->GetEdgeData()->GetAbstractArray(this->EdgePedigreeIdArrayName);
    if (!arr)
      {
      vtkErrorMacro(<< "Could not find edge pedigree id array: " << this->EdgePedigreeIdArrayName);
      return 0;
      }
    output->GetEdgeData()->SetPedigreeIds(arr);
    }
    
  // Done: 100% progress 
  this->UpdateProgress(1);

  return 1;
}

