/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTreeDifferenceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTreeDifferenceFilter.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkTree.h"

//----------------------------------------------------------------------------
int TestTreeDifferenceFilter(int, char*[])
{
  // create tree 1
  vtkNew<vtkMutableDirectedGraph> graph1;
  vtkIdType root = graph1->AddVertex();
  vtkIdType internalOne = graph1->AddChild(root);
  vtkIdType internalTwo = graph1->AddChild(internalOne);
  vtkIdType a = graph1->AddChild(internalTwo);
  vtkIdType b = graph1->AddChild(internalTwo);
  vtkIdType c = graph1->AddChild(internalOne);

  vtkNew<vtkDoubleArray> weights1;
  weights1->SetNumberOfTuples(5);
  weights1->SetValue(graph1->GetEdgeId(root, internalOne), 1.0f);
  weights1->SetValue(graph1->GetEdgeId(internalOne, internalTwo), 2.0f);
  weights1->SetValue(graph1->GetEdgeId(internalTwo, a), 1.0f);
  weights1->SetValue(graph1->GetEdgeId(internalTwo, b), 1.0f);
  weights1->SetValue(graph1->GetEdgeId(internalOne, c), 3.0f);
  weights1->SetName("weight");
  graph1->GetEdgeData()->AddArray(weights1.GetPointer());

  vtkNew<vtkStringArray> names1;
  names1->SetNumberOfTuples(6);
  names1->SetValue(a, "a");
  names1->SetValue(b, "b");
  names1->SetValue(c, "c");
  names1->SetName("node name");
  graph1->GetVertexData()->AddArray(names1.GetPointer());

  vtkNew<vtkTree> tree1;
  tree1->ShallowCopy(graph1.GetPointer());

  // create tree 2.  Same topology as tree 1, but its vertices are created in
  // a different order.  Also, its edge weights are different.
  vtkNew<vtkMutableDirectedGraph> graph2;
  root = graph2->AddVertex();
  internalOne = graph2->AddChild(root);
  c = graph2->AddChild(internalOne);
  internalTwo = graph2->AddChild(internalOne);
  b = graph2->AddChild(internalTwo);
  a = graph2->AddChild(internalTwo);

  vtkNew<vtkStringArray> names2;
  names2->SetNumberOfTuples(6);
  names2->SetValue(a, "a");
  names2->SetValue(b, "b");
  names2->SetValue(c, "c");
  names2->SetName("node name");
  graph2->GetVertexData()->AddArray(names2.GetPointer());

  vtkNew<vtkDoubleArray> weights2;
  weights2->SetNumberOfTuples(5);
  weights2->SetValue(graph2->GetEdgeId(root, internalOne), 2.0f);
  weights2->SetValue(graph2->GetEdgeId(internalOne, internalTwo), 4.0f);
  weights2->SetValue(graph2->GetEdgeId(internalTwo, a), 4.0f);
  weights2->SetValue(graph2->GetEdgeId(internalTwo, b), 5.0f);
  weights2->SetValue(graph2->GetEdgeId(internalOne, c), 8.0f);
  weights2->SetName("weight");
  graph2->GetEdgeData()->AddArray(weights2.GetPointer());

  vtkNew<vtkTree> tree2;
  tree2->ShallowCopy(graph2.GetPointer());

  vtkNew<vtkTreeDifferenceFilter> filter;
  filter->Print(std::cout);
  filter->SetInputDataObject(0, tree1.GetPointer());
  filter->SetInputDataObject(1, tree2.GetPointer());
  filter->SetIdArrayName("node name");
  filter->SetComparisonArrayIsVertexData(false);
  filter->SetComparisonArrayName("weight");
  filter->SetOutputArrayName("weight differences");

  filter->Update();

  vtkNew<vtkTree> outputTree;
  outputTree->ShallowCopy(filter->GetOutput());
  vtkDoubleArray *diff = vtkArrayDownCast<vtkDoubleArray>(
    outputTree->GetEdgeData()->GetAbstractArray("weight differences"));

  if (diff->GetValue(0) != -1.0)
  {
    return EXIT_FAILURE;
  }
  if (diff->GetValue(1) != -2.0)
  {
    return EXIT_FAILURE;
  }
  if (diff->GetValue(2) != -3.0)
  {
    return EXIT_FAILURE;
  }
  if (diff->GetValue(3) != -4.0)
  {
    return EXIT_FAILURE;
  }
  if (diff->GetValue(4) != -5.0)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
