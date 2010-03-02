/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGraph.cxx

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

#include "vtkAdjacentVertexIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkDirectedAcyclicGraph.h"
#include "vtkEdgeListIterator.h"
#include "vtkIdTypeArray.h"
#include "vtkInEdgeIterator.h"
#include "vtkIntArray.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkOutEdgeIterator.h"
#include "vtkSmartPointer.h"
#include "vtkTree.h"
#include "vtkVertexListIterator.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

void TestGraphIterators(vtkGraph *g, int & errors)
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
    vtkEdgeType e = edges->Next();
    if (g->GetSourceVertex(e.Id) != e.Source)
      {
      cerr << "ERROR: Source does not match ("
           << g->GetSourceVertex(e.Id) << "!=" << e.Source << ")" << endl;
      ++errors;
      }
    if (g->GetTargetVertex(e.Id) != e.Target)
      {
      cerr << "ERROR: Target does not match ("
           << g->GetTargetVertex(e.Id) << "!=" << e.Target << ")" << endl;
      ++errors;
      }
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
    vtkIdType index = 0;
    while (outEdges->HasNext())
      {
      vtkOutEdgeType e = outEdges->Next();
      vtkOutEdgeType e2 = g->GetOutEdge(v, index);
      ++numEdges;
      // Count self-loops twice, to ensure all edges are counted twice.
      if (vtkUndirectedGraph::SafeDownCast(g) && v == e.Target)
        {
        ++numEdges;
        }
      if (e.Id != e2.Id)
        {
        cerr << "ERROR: Random-access id != iterator id "
             << e.Id << "!=" << e2.Id << endl;
        ++errors;
        }
      if (e.Target != e2.Target)
        {
        cerr << "ERROR: Random-access target != iterator target "
             << e.Target << "!=" << e2.Target << endl;
        ++errors;
        }
      ++index;
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
    vtkIdType index = 0;
    while (inEdges->HasNext())
      {
      vtkInEdgeType e = inEdges->Next();
      vtkInEdgeType e2 = g->GetInEdge(v, index);
      ++numEdges;
      // Count self-loops twice, to ensure all edges are counted twice.
      if (vtkUndirectedGraph::SafeDownCast(g) && v == e.Source)
        {
        ++numEdges;
        }
      if (e.Id != e2.Id)
        {
        cerr << "ERROR: Random-access id != iterator id "
             << e.Id << "!=" << e2.Id << endl;
        ++errors;
        }
      if (e.Source != e2.Source)
        {
        cerr << "ERROR: Random-access source != iterator source "
             << e.Source << "!=" << e2.Source << endl;
        ++errors;
        }
      ++index;
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

void TestGraphDeletion(int & errors)
{
  //         <-------e0--------
  // ( e4 ) v0 -e3-> v1 -e1-> v2 ( e2 )
  //           <-e5-
  VTK_CREATE(vtkMutableDirectedGraph, mdg);
  mdg->AddVertex();
  mdg->AddVertex();
  mdg->AddVertex();
  mdg->AddEdge(2, 0);
  mdg->AddEdge(1, 2);
  mdg->AddEdge(2, 2);
  mdg->AddEdge(0, 1);
  mdg->AddEdge(0, 0);
  mdg->AddEdge(1, 0);
  VTK_CREATE(vtkIntArray, varr);
  varr->SetName("id");
  varr->InsertNextValue(0);
  varr->InsertNextValue(1);
  varr->InsertNextValue(2);
  VTK_CREATE(vtkIntArray, earr);
  earr->SetName("id");
  earr->InsertNextValue(0);
  earr->InsertNextValue(1);
  earr->InsertNextValue(2);
  earr->InsertNextValue(3);
  earr->InsertNextValue(4);
  earr->InsertNextValue(5);
  mdg->GetVertexData()->AddArray(varr);
  mdg->GetEdgeData()->AddArray(earr);

  // Cause edge list to be built.
  mdg->GetSourceVertex(0);
  mdg->Dump();

  // ( e4 ) v0 -e3-> v1 -e1-> v2 ( e2 )
  //           <-e0-
  mdg->RemoveEdge(0);
  mdg->Dump();
  if (mdg->GetNumberOfEdges() != 5 || mdg->GetSourceVertex(0) != 1 || mdg->GetTargetVertex(0) != 0)
    {
    cerr << "ERROR: Did not remove edge correctly." << endl;
    ++errors;
    }
  if (earr->GetNumberOfTuples() != 5 || earr->GetValue(0) != 5)
    {
    cerr << "ERROR: Did not remove edge property correctly." << endl;
    ++errors;
    }

  // ( e0 ) v0 -e3-> v1 -e1-> v2 ( e2 )
  mdg->RemoveEdge(0);
  mdg->Dump();
  if (mdg->GetNumberOfEdges() != 4 || mdg->GetSourceVertex(0) != 0 || mdg->GetTargetVertex(0) != 0)
    {
    cerr << "ERROR: Did not remove loop correctly." << endl;
    ++errors;
    }
  if (earr->GetNumberOfTuples() != 4 || earr->GetValue(0) != 4)
    {
    cerr << "ERROR: Did not remove loop property correctly." << endl;
    ++errors;
    }

  //                 v1 -e1-> v0 ( e0 )
  mdg->RemoveVertex(0);
  mdg->Dump();
  if (mdg->GetNumberOfVertices() != 2 || mdg->GetNumberOfEdges() != 2 || mdg->GetSourceVertex(0) != 0 || mdg->GetTargetVertex(0) != 0)
    {
    cerr << "ERROR: Did not remove vertex correctly." << endl;
    ++errors;
    }
  if (varr->GetNumberOfTuples() != 2 || varr->GetValue(0) != 2 || varr->GetValue(1) != 1)
    {
    cerr << "ERROR: Did not remove vertex property correctly." << endl;
    ++errors;
    }

  // (empty graph)
  VTK_CREATE(vtkIdTypeArray, removeVertices);
  removeVertices->InsertNextValue(1);
  removeVertices->InsertNextValue(0);
  mdg->RemoveVertices(removeVertices);
  mdg->Dump();
  if (mdg->GetNumberOfVertices() != 0 || mdg->GetNumberOfEdges() != 0)
    {
    cerr << "ERROR: Remove vertices did not work properly." << endl;
    ++errors;
    }

  VTK_CREATE(vtkMutableUndirectedGraph, mug);
  mug->AddVertex();
  mug->AddVertex();
  mug->AddVertex();
  mug->AddEdge(0, 1);
  mug->AddEdge(0, 0);
  mug->AddEdge(2, 0);
  mug->AddEdge(2, 1);
  mug->AddEdge(1, 2);
  VTK_CREATE(vtkIntArray, varr2);
  varr2->InsertNextValue(0);
  varr2->InsertNextValue(1);
  varr2->InsertNextValue(2);
  VTK_CREATE(vtkIntArray, earr2);
  earr2->InsertNextValue(0);
  earr2->InsertNextValue(1);
  earr2->InsertNextValue(2);
  earr2->InsertNextValue(3);
  earr2->InsertNextValue(4);
  mug->GetVertexData()->AddArray(varr2);
  mug->GetEdgeData()->AddArray(earr2);

  // Cause edge list to be built.
  mug->GetSourceVertex(0);
  mug->Dump();

  VTK_CREATE(vtkIdTypeArray, removeEdges);
  removeEdges->InsertNextValue(3);
  removeEdges->InsertNextValue(2);
  removeEdges->InsertNextValue(4);
  removeEdges->InsertNextValue(1);
  removeEdges->InsertNextValue(0);
  mug->RemoveEdges(removeEdges);
  mug->Dump();
  if (mug->GetNumberOfVertices() != 3 || mug->GetNumberOfEdges() != 0)
    {
    cerr << "ERROR: Remove edges did not work properly." << endl;
    ++errors;
    }
  if (earr2->GetNumberOfTuples() != 0)
    {
    cerr << "ERROR: Remove edges properties did not work properly." << endl;
    ++errors;
    }
}

int TestGraph(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int errors = 0;

  VTK_CREATE(vtkMutableDirectedGraph, mdgTree);
  VTK_CREATE(vtkMutableDirectedGraph, mdgNotTree);
  VTK_CREATE(vtkMutableUndirectedGraph, mug);
  VTK_CREATE(vtkDirectedGraph, dg);
  VTK_CREATE(vtkUndirectedGraph, ug);
  VTK_CREATE(vtkTree, t);
  VTK_CREATE(vtkDirectedAcyclicGraph, dag);

  for (vtkIdType i = 0; i < 10; ++i)
    {
    mdgTree->AddVertex();
    mdgNotTree->AddVertex();
    mug->AddVertex();
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

  // Not a tree or DAG since 8 and 9 form a disjoint cycle.
  mdgNotTree->AddEdge(0, 1);
  mdgNotTree->AddEdge(0, 2);
  mdgNotTree->AddEdge(0, 3);
  mdgNotTree->AddEdge(1, 4);
  mdgNotTree->AddEdge(1, 5);
  mdgNotTree->AddEdge(2, 6);
  mdgNotTree->AddEdge(2, 7);
  mdgNotTree->AddEdge(9, 8);
  mdgNotTree->AddEdge(8, 9);

  // Undirected graph with parallel edges
  // and self-loops.
  mug->AddEdge(0, 0);
  mug->AddEdge(0, 1);
  mug->AddEdge(1, 0);
  mug->AddEdge(1, 2);
  mug->AddEdge(1, 3);
  mug->AddEdge(4, 5);
  mug->AddEdge(4, 5);
  mug->AddEdge(6, 7);
  mug->AddEdge(7, 7);

  cerr << "Testing graph conversions ..." << endl;
  if (!t->CheckedShallowCopy(mdgTree))
    {
    cerr << "ERROR: Cannot set valid tree." << endl;
    ++errors;
    }
  if (t->CheckedShallowCopy(mdgNotTree))
    {
    cerr << "ERROR: Can set invalid directed tree." << endl;
    ++errors;
    }
  if (t->CheckedShallowCopy(mug))
    {
    cerr << "ERROR: Can set invalid undirected tree." << endl;
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
  if (dg->CheckedShallowCopy(mug))
    {
    cerr << "ERROR: Can set undirected graph to directed graph." << endl;
    ++errors;
    }
  if (!ug->CheckedShallowCopy(mug))
    {
    cerr << "ERROR: Cannot set valid undirected graph." << endl;
    ++errors;
    }
  if (ug->CheckedShallowCopy(t))
    {
    cerr << "ERROR: Can set tree to undirected graph." << endl;
    ++errors;
    }
  if (ug->CheckedShallowCopy(mdgTree))
    {
    cerr << "ERROR: Can set directed graph to undirected graph." << endl;
    ++errors;
    }
  if (!dag->CheckedShallowCopy(mdgTree))
    {
    cerr << "ERROR: Cannot set valid DAG." << endl;
    ++errors;
    }
  if (dag->CheckedShallowCopy(mdgNotTree))
    {
    cerr << "ERROR: Can set invalid DAG." << endl;
    ++errors;
    }
  if (dag->CheckedShallowCopy(mug))
    {
    cerr << "ERROR: Can set undirected graph to DAG." << endl;
    ++errors;
    }
  cerr << "... done." << endl;

  cerr << "Testing basic graph structure ..." << endl;
  TestGraphIterators(mdgTree, errors);
  TestGraphIterators(mdgNotTree, errors);
  TestGraphIterators(mug, errors);
  TestGraphIterators(dg, errors);
  TestGraphIterators(ug, errors);
  TestGraphIterators(t, errors);
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

  cerr << "Testing graph deletion ..." << endl;
  TestGraphDeletion(errors);
  cerr << "... done." << endl;

  return errors;
}
