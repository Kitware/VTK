/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestKCorePartitioners.cxx

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
#include "vtkSmartPointer.h"
#include "vtkKCoreDecomposition.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestKCoreDecomposition(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  VTK_CREATE(vtkMutableDirectedGraph, dg);
  VTK_CREATE(vtkMutableUndirectedGraph, ug);
  VTK_CREATE(vtkMutableDirectedGraph, sdg);
  VTK_CREATE(vtkMutableUndirectedGraph, sug);
  VTK_CREATE(vtkKCoreDecomposition, kcp);

  int kCores[21] = { 1, 1, 1, 2, 0, 2, 2, 2, 2, 2, 3, 3, 3, 3, 2, 2, 1, 3, 3, 3, 3 };

  // This is the example graph found on page 2 of the paper the k-core algorithm
  // used for the VTK filter implementation.  The test runs it once as a directed graph,
  // and then as an undirected graph. The answer should be the same.
  //
  // An O(m) Algorithm for Cores Decomposition of Networks
  //   V. Batagelj, M. Zaversnik, 2001

  for(int i = 0; i < 21; i++)
    {
    dg->AddVertex();
    }

  dg->AddEdge(0,8);
  dg->AddEdge(1,8);
  dg->AddEdge(2,7);
  dg->AddEdge(3,7);
  dg->AddEdge(3,5);
  dg->AddEdge(5,6);
  dg->AddEdge(6,7);
  dg->AddEdge(8,9);
  dg->AddEdge(8,10);
  dg->AddEdge(9,18);
  dg->AddEdge(9,10);
  dg->AddEdge(10,13);
  dg->AddEdge(10,12);
  dg->AddEdge(10,11);
  dg->AddEdge(11,13);
  dg->AddEdge(11,12);
  dg->AddEdge(12,13);
  dg->AddEdge(13,14);
  dg->AddEdge(13,15);
  dg->AddEdge(13,17);
  dg->AddEdge(14,15);
  dg->AddEdge(15,16);
  dg->AddEdge(15,17);
  dg->AddEdge(17,18);
  dg->AddEdge(17,19);
  dg->AddEdge(17,20);
  dg->AddEdge(18,20);
  dg->AddEdge(18,19);
  dg->AddEdge(19,20);

  kcp->SetInputData(dg);

  kcp->Update();

  vtkDataArray* da = kcp->GetOutput()->GetVertexData()->GetArray("KCoreDecompositionNumbers");

  for(int i=0;i< da->GetNumberOfTuples(); ++i)
    {
    if(da->GetVariantValue(i).ToInt() != kCores[i])
      {
      cerr << "Incorrect k-core value found in directed graph! k-core value is "
           << da->GetVariantValue(i).ToInt() << ", should be " << kCores[i] << endl;
      return 1;
      }
    }

  for(int i = 0; i < 21; i++)
    {
    ug->AddVertex();
    }

  ug->AddEdge(0,8);
  ug->AddEdge(1,8);
  ug->AddEdge(2,7);
  ug->AddEdge(3,7);
  ug->AddEdge(3,5);
  ug->AddEdge(5,6);
  ug->AddEdge(6,7);
  ug->AddEdge(8,9);
  ug->AddEdge(8,10);
  ug->AddEdge(9,18);
  ug->AddEdge(9,10);
  ug->AddEdge(10,13);
  ug->AddEdge(10,12);
  ug->AddEdge(10,11);
  ug->AddEdge(11,13);
  ug->AddEdge(11,12);
  ug->AddEdge(12,13);
  ug->AddEdge(13,14);
  ug->AddEdge(13,15);
  ug->AddEdge(13,17);
  ug->AddEdge(14,15);
  ug->AddEdge(15,16);
  ug->AddEdge(15,17);
  ug->AddEdge(17,18);
  ug->AddEdge(17,19);
  ug->AddEdge(17,20);
  ug->AddEdge(18,20);
  ug->AddEdge(18,19);
  ug->AddEdge(19,20);

  kcp->SetInputData(ug);

  kcp->Update();

  da = kcp->GetOutput()->GetVertexData()->GetArray("KCoreDecompositionNumbers");

  for(int i=0;i< da->GetNumberOfTuples(); ++i)
    {
    if(da->GetVariantValue(i).ToInt() != kCores[i])
      {
      cerr << "Incorrect k-core value found in undirected graph! k-core value is "
           << da->GetVariantValue(i).ToInt() << ", should be " << kCores[i] << endl;
      return 1;
      }
    }

  // Create a small undirected graph to test turning on and off if the graph
  // in and out edges are counted as part of the vertex degree.  Should have
  // no effect on the k-core computation.
  int undirected_kCores[5] = { 1, 1, 1, 1, 0 };

  sug->AddVertex();
  sug->AddVertex();
  sug->AddVertex();
  sug->AddVertex();
  sug->AddVertex();

  sug->AddEdge(3,0);
  sug->AddEdge(0,2);
  sug->AddEdge(1,0);

  kcp->SetInputData(sug);

  kcp->UseInDegreeNeighborsOff();

  kcp->Update();

  da = kcp->GetOutput()->GetVertexData()->GetArray("KCoreDecompositionNumbers");

  for(int i=0;i< da->GetNumberOfTuples(); ++i)
    {
    if(da->GetVariantValue(i).ToInt() != undirected_kCores[i])
      {
      cerr << "Incorrect k-core value found in small undirected graph! k-core value is "
           << da->GetVariantValue(i).ToInt() << ", should be " << undirected_kCores[i] << endl;
      return 1;
      }
    }

  // Create a small directed graph to test turning on and off if the graph
  // in and out edges are counted as part of the vertex degree.
  int directed_kCores[5] = { 2, 1, 1, 2, 0 };

  sdg->AddVertex();
  sdg->AddVertex();
  sdg->AddVertex();
  sdg->AddVertex();
  sdg->AddVertex();

  sdg->AddEdge(0,3);
  sdg->AddEdge(3,0);
  sdg->AddEdge(0,2);
  sdg->AddEdge(1,0);

  kcp->SetInputData(sdg);

  kcp->UseInDegreeNeighborsOn();

  kcp->Update();

  da = kcp->GetOutput()->GetVertexData()->GetArray("KCoreDecompositionNumbers");

  for(int i=0;i< da->GetNumberOfTuples(); ++i)
    {
    if(da->GetVariantValue(i).ToInt() != directed_kCores[i])
      {
      cerr << "Incorrect k-core value found in small directed graph! k-core value is "
           << da->GetVariantValue(i).ToInt() << ", should be " << directed_kCores[i] << endl;
      return 1;
      }
    }

  int no_out_edges_directed_kCores[5] = { 1, 0, 1, 1, 0 };

  kcp->UseOutDegreeNeighborsOff();

  kcp->Update();

  da = kcp->GetOutput()->GetVertexData()->GetArray("KCoreDecompositionNumbers");

  for(int i=0;i< da->GetNumberOfTuples(); ++i)
    {
    if(da->GetVariantValue(i).ToInt() != no_out_edges_directed_kCores[i])
      {
      cerr << "Incorrect k-core value found in small directed graph! k-core value is "
           << da->GetVariantValue(i).ToInt() << ", should be " << no_out_edges_directed_kCores[i] << endl;
      return 1;
      }
    }

  int no_in_edges_directed_kCores[5] = { 1, 1, 0, 1, 0 };

  kcp->UseOutDegreeNeighborsOn();
  kcp->UseInDegreeNeighborsOff();

  kcp->Update();

  da = kcp->GetOutput()->GetVertexData()->GetArray("KCoreDecompositionNumbers");

  for(int i=0;i< da->GetNumberOfTuples(); ++i)
    {
    if(da->GetVariantValue(i).ToInt() != no_in_edges_directed_kCores[i])
      {
      cerr << "Incorrect k-core value found in small directed graph! k-core value is "
           << da->GetVariantValue(i).ToInt() << ", should be " << no_in_edges_directed_kCores[i] << endl;
      return 1;
      }
    }

  return 0;
}
