/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataObjectIO.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetAttributes.h"
#include "vtkDataObjectWriter.h"
#include "vtkDirectedGraph.h"
#include "vtkEdgeListIterator.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkGraph.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkRandomGraphSource.h"
#include "vtkSmartPointer.h"
#include "vtkTree.h"
#include "vtkUndirectedGraph.h"
#include "vtkUnstructuredGrid.h"

void InitializeData(vtkDirectedGraph* Data)
{
  vtkRandomGraphSource* const source = vtkRandomGraphSource::New();
  source->SetNumberOfVertices(5);
  source->SetNumberOfEdges(10);
  source->IncludeEdgeWeightsOn();
  source->DirectedOn();
  source->UseEdgeProbabilityOff();
  source->StartWithTreeOff();
  source->AllowSelfLoopsOff();
  source->Update();

  Data->ShallowCopy(source->GetOutput());
  source->Delete();
}

void InitializeData(vtkUndirectedGraph* Data)
{
  vtkRandomGraphSource* const source = vtkRandomGraphSource::New();
  source->SetNumberOfVertices(5);
  source->SetNumberOfEdges(10);
  source->IncludeEdgeWeightsOn();
  source->DirectedOff();
  source->UseEdgeProbabilityOff();
  source->StartWithTreeOff();
  source->AllowSelfLoopsOff();
  source->Update();

  Data->ShallowCopy(source->GetOutput());
  source->Delete();
}

bool CompareData(vtkGraph* Output, vtkGraph* Input)
{
  bool inputDirected = (vtkDirectedGraph::SafeDownCast(Input) != 0);
  bool outputDirected = (vtkDirectedGraph::SafeDownCast(Output) != 0);
  if(inputDirected != outputDirected)
    return false;

  if(Input->GetNumberOfVertices() != Output->GetNumberOfVertices())
    return false;

  if(Input->GetNumberOfEdges() != Output->GetNumberOfEdges())
    return false;

  if(Input->GetVertexData()->GetNumberOfArrays() != Output->GetVertexData()->GetNumberOfArrays())
    return false;

  if(Input->GetEdgeData()->GetNumberOfArrays() != Output->GetEdgeData()->GetNumberOfArrays())
    return false;

  vtkEdgeListIterator *inputEdges = vtkEdgeListIterator::New();
  vtkEdgeListIterator *outputEdges = vtkEdgeListIterator::New();
  while(inputEdges->HasNext())
    {
    vtkEdgeType inputEdge = inputEdges->Next();
    vtkEdgeType outputEdge = outputEdges->Next();
    if(inputEdge.Source != outputEdge.Source)
      return false;

    if(inputEdge.Target != outputEdge.Target)
      return false;

    if(inputEdge.Id != outputEdge.Id)
      return false;
    }
  inputEdges->Delete();
  outputEdges->Delete();

  return true;
}

void InitializeData(vtkTree* Data)
{
  vtkPoints *pts = vtkPoints::New();
  vtkMutableDirectedGraph *g = vtkMutableDirectedGraph::New();
  for (vtkIdType i = 0; i < 5; ++i)
    {
    g->AddVertex();
    pts->InsertNextPoint(i, 0, 0);
    }
  g->AddEdge(2, 0);
  g->AddEdge(0, 1);
  g->AddEdge(0, 3);
  g->AddEdge(0, 4);
  g->SetPoints(pts);

  if (!Data->CheckedShallowCopy(g))
    {
    cerr << "Invalid tree structure." << endl;
    }

  g->Delete();
  pts->Delete();
}

bool CompareData(vtkTree* Output, vtkTree* Input)
{
  if(Input->GetNumberOfVertices() != Output->GetNumberOfVertices())
    return false;

  if(Input->GetNumberOfEdges() != Output->GetNumberOfEdges())
    return false;

  if(Input->GetVertexData()->GetNumberOfArrays() != Output->GetVertexData()->GetNumberOfArrays())
    return false;

  if(Input->GetEdgeData()->GetNumberOfArrays() != Output->GetEdgeData()->GetNumberOfArrays())
    return false;

  if(Input->GetRoot() != Output->GetRoot())
    return false;

  double inx[3];
  double outx[3];
  for(vtkIdType child = 0; child != Input->GetNumberOfVertices(); ++child)
    {
    Input->GetPoint(child, inx);
    Output->GetPoint(child, outx);

    if (inx[0] != outx[0] || inx[1] != outx[1] || inx[2] != outx[2])
      return false;

    if(Input->GetParent(child) != Output->GetParent(child))
      return false;
    }

  return true;
}

template<typename DataT>
bool TestDataObjectSerialization()
{
  DataT* const output_data = DataT::New();
  InitializeData(output_data);

  const char* const filename = output_data->GetClassName();

  vtkGenericDataObjectWriter* const writer = vtkGenericDataObjectWriter::New();
  writer->SetInputData(output_data);
  writer->SetFileName(filename);
  writer->Write();
  writer->Delete();

  vtkGenericDataObjectReader* const reader = vtkGenericDataObjectReader::New();
  reader->SetFileName(filename);
  reader->Update();

  vtkDataObject *obj = reader->GetOutput();
  DataT* const input_data = DataT::SafeDownCast(obj);
  if(!input_data)
    {
    reader->Delete();
    output_data->Delete();
    return false;
    }

  const bool result = CompareData(output_data, input_data);

  reader->Delete();
  output_data->Delete();

  return result;
}

int TestDataObjectIO(int /*argc*/, char* /*argv*/[])
{
  int result = 0;

  if(!TestDataObjectSerialization<vtkDirectedGraph>())
    {
    cerr << "Error: failure serializing vtkDirectedGraph" << endl;
    result = 1;
    }
  if(!TestDataObjectSerialization<vtkUndirectedGraph>())
    {
    cerr << "Error: failure serializing vtkUndirectedGraph" << endl;
    result = 1;
    }
  if(!TestDataObjectSerialization<vtkTree>())
    {
    cerr << "Error: failure serializing vtkTree" << endl;
    result = 1;
    }
  return result;
}
