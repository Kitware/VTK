/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLGraphReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkSQLGraphReader.h"

#include "vtkAssignCoordinates.h"
#include "vtkDirectedGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSQLQuery.h"
#include "vtkRowQueryToTable.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkTableToGraph.h"
#include "vtkUndirectedGraph.h"

vtkStandardNewMacro(vtkSQLGraphReader);

vtkSQLGraphReader::vtkSQLGraphReader()
{
  this->Directed = 0;
  this->CollapseEdges = 0;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->VertexQuery = NULL;
  this->EdgeQuery = NULL;
  this->SourceField = 0;
  this->TargetField = 0;
  this->VertexIdField = 0;
  this->XField = 0;
  this->YField = 0;
  this->ZField = 0;
}

vtkSQLGraphReader::~vtkSQLGraphReader()
{
  if (this->VertexQuery != NULL)
    {
    this->VertexQuery->Delete();
    }
  if (this->EdgeQuery != NULL)
    {
    this->EdgeQuery->Delete();
    }
  this->SetSourceField(0);
  this->SetTargetField(0);
  this->SetVertexIdField(0);
  this->SetXField(0);
  this->SetYField(0);
  this->SetZField(0);
}

void vtkSQLGraphReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Directed: " << this->Directed << endl;
  os << indent << "CollapseEdges: " << this->CollapseEdges << endl;
  os << indent << "XField: " << (this->XField ? this->XField : "(null)") << endl;
  os << indent << "YField: " << (this->YField ? this->YField : "(null)") << endl;
  os << indent << "ZField: " << (this->ZField ? this->ZField : "(null)") << endl;
  os << indent << "VertexIdField: " << (this->VertexIdField ? this->VertexIdField : "(null)") << endl;
  os << indent << "SourceField: " << (this->SourceField ? this->SourceField : "(null)") << endl;
  os << indent << "TargetField: " << (this->TargetField ? this->TargetField : "(null)") << endl;
  os << indent << "EdgeQuery: " << (this->EdgeQuery ? "" : "(null)") << endl;
  if (this->EdgeQuery)
    {
    this->EdgeQuery->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "VertexQuery: " << (this->VertexQuery ? "" : "(null)") << endl;
  if (this->VertexQuery)
    {
    this->VertexQuery->PrintSelf(os, indent.GetNextIndent());
    }
}

vtkCxxSetObjectMacro(vtkSQLGraphReader, VertexQuery, vtkSQLQuery);
vtkCxxSetObjectMacro(vtkSQLGraphReader, EdgeQuery, vtkSQLQuery);

int vtkSQLGraphReader::RequestData(
  vtkInformation*, 
  vtkInformationVector** , 
  vtkInformationVector* outputVector)
{
  // Check for valid inputs
  if (this->EdgeQuery == NULL)
    {
    vtkErrorMacro("The EdgeQuery must be defined");
    return 0;
    }
  if (this->SourceField == NULL)
    {
    vtkErrorMacro("The SourceField must be defined");
    return 0;
    }
  if (this->TargetField == NULL)
    {
    vtkErrorMacro("The TargetField must be defined");
    return 0;
    }
  if (this->VertexQuery != NULL)
    {
    if (this->VertexIdField == NULL)
      {
      vtkErrorMacro("The VertexIdField must be defined when using a VertexQuery");
      return 0;
      }
    if (this->XField != NULL && this->YField == NULL)
      {
      vtkErrorMacro("The YField must be defined if the XField is defined");
      return 0;
      }
    }

  vtkGraph* output = vtkGraph::GetData(outputVector);

  vtkTableToGraph* filter = vtkTableToGraph::New();
  filter->SetDirected(this->Directed);

  // Set up the internal filter to use the edge table
  vtkSmartPointer<vtkRowQueryToTable> edgeReader = vtkSmartPointer<vtkRowQueryToTable>::New();
  edgeReader->SetQuery(this->EdgeQuery);
  edgeReader->Update();
  vtkTable* edgeTable = edgeReader->GetOutput();
  
  const char* domain = "default";
  if (this->VertexIdField)
    {
    domain = this->VertexIdField;
    }
  
  filter->SetInput(0, edgeTable);
  filter->AddLinkVertex(this->SourceField, domain);
  filter->AddLinkVertex(this->TargetField, domain);
  filter->AddLinkEdge(this->SourceField, this->TargetField);
  
  vtkSmartPointer<vtkAssignCoordinates> assign = vtkSmartPointer<vtkAssignCoordinates>::New();
  assign->SetInputConnection(filter->GetOutputPort());
  
  // Set up the internal filter to use the vertex table
  if (this->VertexQuery != NULL)
    {
    vtkSmartPointer<vtkRowQueryToTable> vertexReader = vtkSmartPointer<vtkRowQueryToTable>::New();
    vertexReader->SetQuery(this->VertexQuery);
    vertexReader->Update();
    vtkTable* vertexTable = vertexReader->GetOutput();
    filter->SetInput(1, vertexTable);
    if (this->XField != NULL)
      {
      assign->SetXCoordArrayName(this->XField);
      assign->SetYCoordArrayName(this->YField);
      if (this->ZField != NULL)
        {
        assign->SetZCoordArrayName(this->ZField);
        }
      }
    }

  // Get the internal filter output and assign it to the output
  if (this->XField != NULL)
    {
    assign->Update();
    vtkGraph* assignOutput = vtkGraph::SafeDownCast(assign->GetOutput());
    output->ShallowCopy(assignOutput);
    }
  else
    {
    filter->Update();
    vtkGraph* filterOutput = filter->GetOutput();
    output->ShallowCopy(filterOutput);
    }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int piece = -1;
  int npieces = -1;
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    npieces = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    }
  output->GetInformation()->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), npieces);
  output->GetInformation()->Set(vtkDataObject::DATA_PIECE_NUMBER(), piece);

  // Clean up
  filter->Delete();

  return 1;
}

int vtkSQLGraphReader::RequestDataObject(
  vtkInformation*, 
  vtkInformationVector** , 
  vtkInformationVector*)
{
  vtkDataObject *current = this->GetExecutive()->GetOutputData(0);
  if (!current 
    || (this->Directed && !vtkDirectedGraph::SafeDownCast(current))
    || (!this->Directed && vtkDirectedGraph::SafeDownCast(current)))
    {
    vtkGraph *output = 0;
    if (this->Directed)
      {
      output = vtkDirectedGraph::New();
      }
    else
      {
      output = vtkUndirectedGraph::New();
      }
    this->GetExecutive()->SetOutputData(0, output);
    output->Delete();
    }

  return 1;
}
