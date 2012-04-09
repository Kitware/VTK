/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PBGLNamedVertexGraph.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */

#include <mpi.h>

#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkEdgeListIterator.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkVertexListIterator.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

// Percentage of the time that the AddEdge operation in this test will
// perform an "immediate" edge-addition operation, requiring the
// processor initiating the AddEdge call to wait until the owner of
// the edge has actually added the edge.
static const int ImmediateAddEdgeChance = 3;


#define myassert(Cond)                                  \
  if (!(Cond))                                          \
    {                                                   \
      cerr << "error (" __FILE__ ":" << dec << __LINE__ \
           << ") assertion \"" #Cond "\" failed."       \
           << endl;                                     \
    MPI_Abort(MPI_COMM_WORLD, -1);                      \
    }

void TestNamedUndirectedGraph()
{
  // Create the distributed graph
  vtkSmartPointer<vtkMutableUndirectedGraph>
    graph = vtkSmartPointer<vtkMutableUndirectedGraph>::New();
  vtkSmartPointer<vtkPBGLDistributedGraphHelper> helper
    = vtkSmartPointer<vtkPBGLDistributedGraphHelper>::New();
  graph->SetDistributedGraphHelper(helper);

  int rank = graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  int numProcs
    = graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  // Make it a graph with the pedigree IDs vertices
  vtkSmartPointer<vtkVariantArray> pedigreeIds
    = vtkSmartPointer<vtkVariantArray>::New();
  graph->GetVertexData()->SetPedigreeIds(pedigreeIds);
  helper->Synchronize();

  // Build the graph itself.
  if (rank == 0)
    {
    graph->AddEdge("Bloomington", "Indianapolis");
    graph->AddEdge("Indianapolis", "Chicago");
    }
  else if (rank == numProcs - 1)
    {
    graph->AddEdge("Indianapolis", "Cincinnati");
    graph->AddEdge("Indianapolis", "Louisville");
    }
  helper->Synchronize();

  // Display the vertices (and their names)
  vtkSmartPointer<vtkVertexListIterator> vertices
    = vtkSmartPointer<vtkVertexListIterator>::New();
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType vertex = vertices->Next();
    vtkVariant pedigreeId
      = pedigreeIds->GetValue(helper->GetVertexIndex(vertex));
    cout << "Rank #" << rank << ": vertex " << pedigreeId.ToString() << " ("
         << hex << vertex << ")\n";
    cout.flush();
    }

  // Display the edges
  vtkSmartPointer<vtkEdgeListIterator> edges
    = vtkSmartPointer<vtkEdgeListIterator>::New();
  graph->GetEdges(edges);
  while (edges->HasNext())
    {
    vtkEdgeType edge = edges->Next();
    cout << "Rank #" << rank << ": edge (" << hex << edge.Source << ", "
         << edge.Target << ")\n";
    cout.flush();
    }
}

void TestWithStringArray()
{
  // Create the distributed graph
  vtkSmartPointer<vtkMutableUndirectedGraph>
    graph = vtkSmartPointer<vtkMutableUndirectedGraph>::New();
  vtkSmartPointer<vtkPBGLDistributedGraphHelper> helper
    = vtkSmartPointer<vtkPBGLDistributedGraphHelper>::New();
  graph->SetDistributedGraphHelper(helper);

  int rank = graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  int numProcs
    = graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  // Make it a graph with the pedigree IDs vertices
  vtkSmartPointer<vtkStringArray> pedigreeIds
    = vtkSmartPointer<vtkStringArray>::New();
  graph->GetVertexData()->SetPedigreeIds(pedigreeIds);
  helper->Synchronize();

  // Build the graph itself.
  if (rank == 0)
    {
    graph->AddEdge("Bloomington", "Indianapolis");
    graph->AddEdge("Indianapolis", "Chicago");
    }
  else if (rank == numProcs - 1)
    {
    graph->AddEdge("Indianapolis", "Cincinnati");
    graph->AddEdge("Indianapolis", "Louisville");
    }
  helper->Synchronize();

  // Display the vertices (and their names)
  vtkSmartPointer<vtkVertexListIterator> vertices
    = vtkSmartPointer<vtkVertexListIterator>::New();
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType vertex = vertices->Next();
    vtkStdString pedigreeId
      = pedigreeIds->GetValue(helper->GetVertexIndex(vertex));
    cout << "Rank #" << rank << ": vertex " << pedigreeId << " ("
         << hex << vertex << ")\n";
    cout.flush();
    }

  // Display the edges
  vtkSmartPointer<vtkEdgeListIterator> edges
    = vtkSmartPointer<vtkEdgeListIterator>::New();
  graph->GetEdges(edges);
  while (edges->HasNext())
    {
    vtkEdgeType edge = edges->Next();
    cout << "Rank #" << rank << ": edge (" << hex << edge.Source << ", "
         << edge.Target << ")\n";
    cout.flush();
    }
}

void TestWithIntArray()
{
  // Create the distributed graph
  vtkSmartPointer<vtkMutableUndirectedGraph>
    graph = vtkSmartPointer<vtkMutableUndirectedGraph>::New();
  vtkSmartPointer<vtkPBGLDistributedGraphHelper> helper
    = vtkSmartPointer<vtkPBGLDistributedGraphHelper>::New();
  graph->SetDistributedGraphHelper(helper);

  int rank = graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  int numProcs
    = graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  // Make it a graph with the pedigree IDs vertices
  vtkSmartPointer<vtkIntArray> pedigreeIds
    = vtkSmartPointer<vtkIntArray>::New();
  graph->GetVertexData()->SetPedigreeIds(pedigreeIds);
  helper->Synchronize();

  // Build the graph itself.
  if (rank == 0)
    {
    graph->AddEdge(vtkVariant(17), vtkVariant(42));
    graph->AddEdge(vtkVariant(42), vtkVariant(19));
    }
  else if (rank == numProcs - 1)
    {
    graph->AddEdge(vtkVariant(42), vtkVariant(11));
    graph->AddEdge(vtkVariant(42), vtkVariant(13));
    }
  helper->Synchronize();

  // Display the vertices (and their names)
  vtkSmartPointer<vtkVertexListIterator> vertices
    = vtkSmartPointer<vtkVertexListIterator>::New();
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType vertex = vertices->Next();
    int pedigreeId = pedigreeIds->GetValue(helper->GetVertexIndex(vertex));
    cout << "Rank #" << rank << ": vertex " << dec << pedigreeId << " ("
         << hex << vertex << ")\n";
    cout.flush();
    }

  // Display the edges
  vtkSmartPointer<vtkEdgeListIterator> edges
    = vtkSmartPointer<vtkEdgeListIterator>::New();
  graph->GetEdges(edges);
  while (edges->HasNext())
    {
    vtkEdgeType edge = edges->Next();
    cout << "Rank #" << rank << ": edge (" << hex << edge.Source << ", "
         << edge.Target << ")\n";
    cout.flush();
    }
}

//----------------------------------------------------------------------------
int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  TestNamedUndirectedGraph();
  TestWithStringArray();
  TestWithIntArray();
  MPI_Finalize();
  return 0;
}
