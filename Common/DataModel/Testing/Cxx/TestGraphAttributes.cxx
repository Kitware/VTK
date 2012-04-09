/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGraphAttributes.cxx

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

#include "vtkAdjacentVertexIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkDirectedAcyclicGraph.h"
#include "vtkEdgeListIterator.h"
#include "vtkInEdgeIterator.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkOutEdgeIterator.h"
#include "vtkSmartPointer.h"
#include "vtkTree.h"
#include "vtkVertexListIterator.h"
#include "vtkVariantArray.h"
#include "vtkStringArray.h"
#include "vtkIntArray.h"


#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

void TestGraphAttribIterators(vtkGraph *g, int & errors)
{
  if (g->GetNumberOfVertices() != 10)
    {
    cerr << "ERROR: Wrong number of vertices." << endl;
    ++errors;
    }
  if (g->GetNumberOfEdges() != 9)
    {
    cerr << "ERROR: Wrong number of edges." << endl;
    ++errors;
    }
  VTK_CREATE(vtkVertexListIterator, vertices);
  g->GetVertices(vertices);
  vtkIdType numVertices = 0;
  while (vertices->HasNext())
    {
    vertices->Next();
    ++numVertices;
    }
  if (numVertices != 10)
    {
    cerr << "ERROR: Vertex list iterator failed." << endl;
    ++errors;
    }
  VTK_CREATE(vtkEdgeListIterator, edges);
  g->GetEdges(edges);
  vtkIdType numEdges = 0;
  while (edges->HasNext())
    {
    edges->Next();
    ++numEdges;
    }
  if (numEdges != 9)
    {
    cerr << "ERROR: Edge list iterator failed." << endl;
    ++errors;
    }
  numEdges = 0;
  VTK_CREATE(vtkOutEdgeIterator, outEdges);
  g->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType v = vertices->Next();
    g->GetOutEdges(v, outEdges);
    while (outEdges->HasNext())
      {
      vtkOutEdgeType e = outEdges->Next();
      ++numEdges;
      // Count self-loops twice, to ensure all edges are counted twice.
      if (vtkUndirectedGraph::SafeDownCast(g) && v == e.Target)
        {
        ++numEdges;
        }
      }
    }
  if (vtkDirectedGraph::SafeDownCast(g) && numEdges != 9)
    {
    cerr << "ERROR: Out edge iterator failed." << endl;
    ++errors;
    }
  if (vtkUndirectedGraph::SafeDownCast(g) && numEdges != 18)
    {
    cerr << "ERROR: Undirected out edge iterator failed." << endl;
    ++errors;
    }
  numEdges = 0;
  VTK_CREATE(vtkInEdgeIterator, inEdges);
  g->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType v = vertices->Next();
    g->GetInEdges(v, inEdges);
    while (inEdges->HasNext())
      {
      vtkInEdgeType e = inEdges->Next();
      ++numEdges;
      // Count self-loops twice, to ensure all edges are counted twice.
      if (vtkUndirectedGraph::SafeDownCast(g) && v == e.Source)
        {
        ++numEdges;
        }
      }
    }
  if (vtkDirectedGraph::SafeDownCast(g) && numEdges != 9)
    {
    cerr << "ERROR: In edge iterator failed." << endl;
    ++errors;
    }
  if (vtkUndirectedGraph::SafeDownCast(g) && numEdges != 18)
    {
    cerr << "ERROR: Undirected in edge iterator failed." << endl;
    ++errors;
    }
  numEdges = 0;
  VTK_CREATE(vtkAdjacentVertexIterator, adjacent);
  g->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType v = vertices->Next();
    g->GetAdjacentVertices(v, adjacent);
    while (adjacent->HasNext())
      {
      vtkIdType u = adjacent->Next();
      ++numEdges;
      // Count self-loops twice, to ensure all edges are counted twice.
      if (vtkUndirectedGraph::SafeDownCast(g) && v == u)
        {
        ++numEdges;
        }
      }
    }
  if (vtkDirectedGraph::SafeDownCast(g) && numEdges != 9)
    {
    cerr << "ERROR: In edge iterator failed." << endl;
    ++errors;
    }
  if (vtkUndirectedGraph::SafeDownCast(g) && numEdges != 18)
    {
    cerr << "ERROR: Undirected in edge iterator failed." << endl;
    ++errors;
    }
}

int TestGraphAttributes(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int errors = 0;

  VTK_CREATE(vtkMutableDirectedGraph, mdgTree);

  VTK_CREATE(vtkDirectedGraph, dg);
  VTK_CREATE(vtkTree, t);

  //  Create some vertex property arrays
  VTK_CREATE(vtkVariantArray, vertexPropertyArr);
  int numVertexProperties = 2;
  vertexPropertyArr->SetNumberOfValues(numVertexProperties);

  VTK_CREATE(vtkStringArray, vertexProp0Array);
  vertexProp0Array->SetName("labels");
  mdgTree->GetVertexData()->AddArray(vertexProp0Array);

  VTK_CREATE(vtkIntArray, vertexProp1Array);
  vertexProp1Array->SetName("weight");
  mdgTree->GetVertexData()->AddArray(vertexProp1Array);

  const char *vertexLabel[] = {"Dick","Jane","Sally","Spot","Puff"};

  const char *stringProp;
  int weight;

  for (vtkIdType i = 0; i < 10; ++i)
    {
    stringProp = vertexLabel[rand() % 5];
    weight = rand() % 10;
//    cout << myRank <<" vertex "<< v <<","<< stringProp <<","<<weight<< endl;
    vertexPropertyArr->SetValue(0,stringProp);
    vertexPropertyArr->SetValue(1,weight);
    mdgTree->AddVertex(vertexPropertyArr);

    }

  // Create a valid tree.
  mdgTree->AddEdge(0, 1);
  mdgTree->AddEdge(0, 2);
  mdgTree->AddEdge(0, 3);
  mdgTree->AddEdge(1, 4);
  mdgTree->AddEdge(1, 5);
  mdgTree->AddEdge(2, 6);
  mdgTree->AddEdge(2, 7);
  mdgTree->AddEdge(3, 8);
  mdgTree->AddEdge(3, 9);


  cerr << "Testing graph conversions ..." << endl;
  if (!t->CheckedShallowCopy(mdgTree))
    {
    cerr << "ERROR: Cannot set valid tree." << endl;
    ++errors;
    }

  if (!dg->CheckedShallowCopy(mdgTree))
    {
    cerr << "ERROR: Cannot set valid directed graph." << endl;
    ++errors;
    }
  if (!dg->CheckedShallowCopy(t))
    {
    cerr << "ERROR: Cannot set tree to directed graph." << endl;
    ++errors;
    }

  cerr << "... done." << endl;

  cerr << "Testing basic graph structure ..." << endl;
  TestGraphAttribIterators(mdgTree, errors);
  TestGraphAttribIterators(dg, errors);
  TestGraphAttribIterators(t, errors);
  cerr << "... done." << endl;

  cerr << "Testing copy on write ..." << endl;
  if (!t->IsSameStructure(mdgTree))
    {
    cerr << "ERROR: Tree and directed graph should be sharing the same structure." << endl;
    ++errors;
    }
  mdgTree->AddVertex();
  if (t->IsSameStructure(mdgTree))
    {
    cerr << "ERROR: Tree and directed graph should not be sharing the same structure." << endl;
    ++errors;
    }
  if (t->GetNumberOfVertices() != 10)
    {
    cerr << "ERROR: Tree changed when modifying directed graph." << endl;
    ++errors;
    }
  cerr << "... done." << endl;

  return errors;
}
