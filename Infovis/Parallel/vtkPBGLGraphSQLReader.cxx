/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLGraphSQLReader.cxx

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

#include "vtkPBGLGraphSQLReader.h"

#if !defined(VTK_LEGACY_REMOVE)

#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkSmartPointer.h"
#include "vtkSQLDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkVariantArray.h"

#include <vtksys/ios/sstream>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkPBGLGraphSQLReader);



vtkIdType IdentityDistribution(const vtkVariant& id, void* user_data)
{
  vtkIdType* data = (vtkIdType*)user_data;
  vtkIdType min, size;
  int num_procs = static_cast<int>(data[0]);
  int val = id.ToInt() - 1;
  for (vtkIdType i = 0; i < num_procs; ++i)
    {
    vtkPBGLGraphSQLReader::GetRange(i, num_procs, data[1], min, size);
    if (val >= min && val < min + size)
      {
      return i;
      }
    }
  //return static_cast<vtkIdType>(*((int*)user_data));
  //return id.ToInt() % 2;
  return 0;
}



vtkPBGLGraphSQLReader::vtkPBGLGraphSQLReader()
{
  this->Directed = true;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->VertexTable = 0;
  this->EdgeTable = 0;
  this->SourceField = 0;
  this->TargetField = 0;
  this->VertexIdField = 0;
  this->Database = 0;
  this->DistributionUserData[0] = 0;
  this->DistributionUserData[1] = 0;

  VTK_LEGACY_BODY(vtkPBGLGraphSQLReader::vtkPBGLGraphSQLReader, "VTK 6.2");
}



vtkPBGLGraphSQLReader::~vtkPBGLGraphSQLReader()
{
  this->SetDatabase(0);
  this->SetVertexTable(0);
  this->SetEdgeTable(0);
  this->SetSourceField(0);
  this->SetTargetField(0);
  this->SetVertexIdField(0);
}



void vtkPBGLGraphSQLReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Directed: " << this->Directed << endl;
  os << indent << "VertexIdField: " << (this->VertexIdField ? this->VertexIdField : "(null)") << endl;
  os << indent << "SourceField: " << (this->SourceField ? this->SourceField : "(null)") << endl;
  os << indent << "TargetField: " << (this->TargetField ? this->TargetField : "(null)") << endl;
  os << indent << "EdgeTable: " << (this->EdgeTable ? this->EdgeTable : "(null)") << endl;
  os << indent << "VertexTable: " << (this->VertexTable ? this->VertexTable : "(null)") << endl;
  os << indent << "Database: " << (this->Database ? "" : "(null)") << endl;
  if (this->Database)
    {
    this->Database->PrintSelf(os, indent.GetNextIndent());
    }
}



vtkCxxSetObjectMacro(vtkPBGLGraphSQLReader, Database, vtkSQLDatabase);



template<class MutableGraph>
int vtkPBGLGraphSQLReaderRequestData(
  vtkPBGLGraphSQLReader* self,
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector* output_vec)
{
  VTK_CREATE(vtkTimerLog, timer);
  timer->StartTimer();
  // Check for valid inputs
  if (!self->GetDatabase())
    {
    vtkGenericWarningMacro("The Database must be defined");
    return 0;
    }
  if (!self->GetEdgeTable())
    {
    vtkGenericWarningMacro("The EdgeTable must be defined");
    return 0;
    }
  if (!self->GetSourceField())
    {
    vtkGenericWarningMacro("The SourceField must be defined");
    return 0;
    }
  if (!self->GetTargetField())
    {
    vtkGenericWarningMacro("The TargetField must be defined");
    return 0;
    }
  if (!self->GetVertexTable())
    {
    vtkGenericWarningMacro("The VertexTable must be defined");
    return 0;
    }
  if (!self->GetVertexIdField())
    {
    vtkGenericWarningMacro("The VertexIdField must be defined");
    return 0;
    }

  vtkGraph* output = vtkGraph::GetData(output_vec);

  // Create directed or undirected graph
  MutableGraph* builder = MutableGraph::New();

  // Set up vertex and edge queries
  vtksys_ios::ostringstream oss;
  oss << "select count(*) from " << self->GetVertexTable();
  vtkSmartPointer<vtkSQLQuery> vertex_count;
  vertex_count.TakeReference(self->GetDatabase()->GetQueryInstance());
  vertex_count->SetQuery(oss.str().c_str());
  vertex_count->Execute();
  vertex_count->NextRow();
  int num_verts = vertex_count->DataValue(0).ToInt();

  oss.str("");
  oss << "select count(*) from " << self->GetEdgeTable();
  vtkSmartPointer<vtkSQLQuery> edge_count;
  edge_count.TakeReference(self->GetDatabase()->GetQueryInstance());
  edge_count->SetQuery(oss.str().c_str());
  edge_count->Execute();
  edge_count->NextRow();
  int num_edges = edge_count->DataValue(0).ToInt();

  vtkInformation* out_info = output_vec->GetInformationObject(0);
  int rank = out_info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int total = out_info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  vtkIdType vert_limit, vert_offset;
  vtkIdType edge_limit, edge_offset;
  self->GetRange(rank, total, num_verts, vert_offset, vert_limit);
  self->GetRange(rank, total, num_edges, edge_offset, edge_limit);

  // Setup the graph as a distributed graph
  VTK_CREATE(vtkPBGLDistributedGraphHelper, helper);
  builder->SetDistributedGraphHelper(helper);

  // -----[ Verts ]-------------------------

  // Read my vertices from vertex query, adding attribute values
  oss.str("");
  oss << "select * from " << self->GetVertexTable()
      << " limit " << vert_limit
      << " offset " << vert_offset;
  vtkSmartPointer<vtkSQLQuery> vertex_query;
  vertex_query.TakeReference(self->GetDatabase()->GetQueryInstance());
  vertex_query->SetQuery(oss.str().c_str());
  vertex_query->Execute();

  // Add local vertex data arrays
  // Note: GetNumberOfFields() is analogous to the # of columns in Query Tbl.
  for (int i = 0; i < vertex_query->GetNumberOfFields(); ++i)
    {
    vtkStdString field_name = vertex_query->GetFieldName(i);
    vtkSmartPointer<vtkAbstractArray> arr;
    arr.TakeReference(vtkAbstractArray::CreateArray(
      vertex_query->GetFieldType(i)));
    arr->SetName(field_name);

    if (field_name == self->GetVertexIdField())
      {
      builder->GetVertexData()->SetPedigreeIds(arr);
      }
    else
      {
      builder->GetVertexData()->AddArray(arr);
      }
    }
  helper->Synchronize();

  // Add the vertices
  VTK_CREATE(vtkVariantArray, row);
  while (vertex_query->NextRow(row))
    {
    builder->LazyAddVertex(row);
    }
  helper->Synchronize();

  // -----[ Edges ]-------------------------

  // Read edges from edge query, adding attribute values
  oss.str("");
  oss << "select * from " << self->GetEdgeTable()
      << " limit " << edge_limit
      << " offset " << edge_offset;
  vtkSmartPointer<vtkSQLQuery> edge_query;
  edge_query.TakeReference(self->GetDatabase()->GetQueryInstance());
  edge_query->SetQuery(oss.str().c_str());
  edge_query->Execute();

#if defined(DEBUG)      // WCMCLEN
  cout << "["<<rank<<"]\tSQLReader: NumberOfEdgeAttributeArrays = "
       << edge_query->GetNumberOfFields() << endl;
  fflush(stdout);
#endif

  // Add local edge data arrays
  for (int i = 0; i < edge_query->GetNumberOfFields(); ++i)
    {
    vtkStdString field_name = edge_query->GetFieldName(i);
    vtkSmartPointer<vtkAbstractArray> arr;
    arr.TakeReference(vtkAbstractArray::CreateArray(edge_query->GetFieldType(i)));
    arr->SetName(field_name);
    builder->GetEdgeData()->AddArray(arr);

#if defined(DEBUG)      // WCMCLEN
    cout << "["<<rank<<"]\t"
         << "-\tEdge field_name["<<i<<"]='"<<field_name<<"'"
         << endl;
    fflush(stdout);
#endif
    }
  helper->Synchronize();

  // Add the edges
  int source_id = edge_query->GetFieldIndex(self->GetSourceField());
  int target_id = edge_query->GetFieldIndex(self->GetTargetField());
  while (edge_query->NextRow(row))
    {
    vtkVariant source = edge_query->DataValue(source_id);
    vtkVariant target = edge_query->DataValue(target_id);

#if defined(DEBUG)      // WCMCLEN
    cout << "["<<rank<< "]\tReader: Read edge("
         << source.ToString() << ", " << target.ToString() << ", " << &row << ")"
         << endl;
    fflush(stdout);
#endif

    builder->LazyAddEdge(source, target, row);
    }

#if defined(DEBUG)      // WCMCLEN
  cout << "["<<rank<<"]\tReader: Done adding edges!" << endl;
  fflush(stdout);
#endif
  helper->Synchronize();

  // Call a second synchronize to pick up anything that was possibly sent
  // in a trigger after the previous synchronize.
  helper->Synchronize();

  // Copy into output graph
  if (!output->CheckedShallowCopy(builder))
    {
    vtkGenericWarningMacro("Could not copy to output.");
    return 0;
    }

  timer->StopTimer();
#ifdef VTKPBGL_REPORT_TIMES
  // Suppress timer printing (for now)
  cerr << "vtkPBGLGraphSQLReader: " << timer->GetElapsedTime() << endl;
#endif

  return 1;
}



int vtkPBGLGraphSQLReader::RequestData(
  vtkInformation* info,
  vtkInformationVector** input_vec,
  vtkInformationVector* output_vec)
{
  int rval = 0;
  if (this->Directed)
    {
    rval = vtkPBGLGraphSQLReaderRequestData<vtkMutableDirectedGraph>(
              this, info, input_vec, output_vec);
    }
  else
    {
    rval = vtkPBGLGraphSQLReaderRequestData<vtkMutableUndirectedGraph>(
              this, info, input_vec, output_vec);
    }
  return rval;
}



int vtkPBGLGraphSQLReader::RequestDataObject(
  vtkInformation* info,
  vtkInformationVector** input_vec,
  vtkInformationVector* output_vec)
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
    vtkInformation* out_info = output_vec->GetInformationObject(0);
    this->GetExecutive()->SetOutputData(0, output);
    output->Delete();
    }

  return 1;
}



void vtkPBGLGraphSQLReader::GetRange(int rank, int total,
  vtkIdType size, vtkIdType& offset, vtkIdType& limit)
{
  offset = size*rank/total;
  limit = size*(rank+1)/total - offset;
}

#endif //VTK_LEGACY_REMOVE
