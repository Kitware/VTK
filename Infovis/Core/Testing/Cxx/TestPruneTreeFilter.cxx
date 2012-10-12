/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPruneTreeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPruneTreeFilter.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkTree.h"

//----------------------------------------------------------------------------
int TestPruneTreeFilter(int, char*[])
{
  vtkNew<vtkMutableDirectedGraph> graph;
  vtkIdType root = graph->AddVertex();
  vtkIdType internalOne = graph->AddChild(root);
  vtkIdType internalTwo = graph->AddChild(internalOne);
  vtkIdType a = graph->AddChild(internalTwo);
  graph->AddChild(internalTwo);
  graph->AddChild(internalOne);
  graph->AddChild(a);
  graph->AddChild(a);

  vtkNew<vtkTree> tree;
  tree->ShallowCopy(graph.GetPointer());

  vtkNew<vtkPruneTreeFilter> filter;
  filter->SetInputData(tree.GetPointer());
  filter->SetParentVertex(internalTwo);
  vtkTree *prunedTree = filter->GetOutput();
  filter->Update();

  if (prunedTree->GetNumberOfVertices() == 3)
    {
    return EXIT_SUCCESS;
    }

  return EXIT_FAILURE;
}
