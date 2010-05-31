/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBoostAlgorithms.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkBoostBetweennessClustering.h"
#include "vtkDataSetAttributes.h"
#include "vtkIntArray.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkVertexListIterator.h"

#include <boost/version.hpp>

#include <vtkstd/map>

int TestBoostBetweennessClustering(int vtkNotUsed(argc),
                                   char* vtkNotUsed(argv)[])
{
  // Create the test graph
  vtkSmartPointer<vtkMutableUndirectedGraph> g
    (vtkSmartPointer<vtkMutableUndirectedGraph>::New());

  vtkSmartPointer<vtkIntArray> weights (
    vtkSmartPointer<vtkIntArray>::New());
  weights->SetName("weights");

  g->GetEdgeData()->AddArray(weights);

  vtkSmartPointer<vtkPoints> pts (vtkSmartPointer<vtkPoints>::New());
  g->AddVertex();
  pts->InsertNextPoint(1, 1, 0);

  g->AddVertex();
  pts->InsertNextPoint(1, 0, 0);

  g->AddVertex();
  pts->InsertNextPoint(1, -1, 0);

  g->AddVertex();
  pts->InsertNextPoint(2, 0, 0);

  g->AddVertex();
  pts->InsertNextPoint(3, 0, 0);

  g->AddVertex();
  pts->InsertNextPoint(2.5, 1, 0);

  g->AddVertex();
  pts->InsertNextPoint(4, 1, 0);

  g->AddVertex();
  pts->InsertNextPoint(4, 0, 0);

  g->AddVertex();
  pts->InsertNextPoint(4, -1, 0);

  g->SetPoints(pts);

  vtkEdgeType e = g->AddEdge(0, 3);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(1, 3);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(2, 3);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(3, 4);
  weights->InsertTuple1(e.Id, 1);

  e = g->AddEdge(3, 5);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(5, 4);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(6, 4);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(7, 4);
  weights->InsertTuple1(e.Id, 10);

  e = g->AddEdge(8, 4);
  weights->InsertTuple1(e.Id, 10);

  // Test centrality
  vtkSmartPointer<vtkBoostBetweennessClustering> bbc (
    vtkSmartPointer<vtkBoostBetweennessClustering>::New());
  bbc->SetInput(g);
  bbc->SetThreshold(4);
  bbc->SetEdgeWeightArrayName("weights");
  bbc->SetEdgeCentralityArrayName("bbc_centrality");
  bbc->UseEdgeWeightArrayOn();
  bbc->Update();

  vtkGraph* og = bbc->GetOutput();

  if(!og)
    {
    return 1;
    }

  vtkIntArray* compArray = vtkIntArray::SafeDownCast(og->GetVertexData()->
                                                     GetArray("component"));
  if(!compArray)
    {
    return 1;
    }

  // Now lets create the correct mapping so that we can compare the results
  // against it.
  vtkstd::map<int, int> expResults;
  expResults[0] = 0;
  expResults[1] = 0;
  expResults[2] = 0;
  expResults[3] = 0;
  expResults[4] = 1;
  expResults[5] = 1;
  expResults[6] = 1;
  expResults[7] = 1;
  expResults[8] = 2;

  vtkSmartPointer<vtkVertexListIterator> vlItr (
    vtkSmartPointer<vtkVertexListIterator>::New());
  vlItr->SetGraph(og);

  while(vlItr->HasNext())
    {
    vtkIdType id = vlItr->Next();

    if(expResults[id] != compArray->GetVariantValue(id).ToInt())
      {
      return 1;
      }
    }

    return 0;
}
