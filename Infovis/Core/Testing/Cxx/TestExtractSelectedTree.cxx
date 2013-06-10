/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractSelectedTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkExtractSelectedTree.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkTree.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"

//----------------------------------------------------------------------------
int TestExtractSelectedTree(int, char*[])
{
  vtkNew<vtkMutableDirectedGraph> graph;
  vtkIdType root = graph->AddVertex();
  vtkIdType internalOne = graph->AddChild(root);
  vtkIdType internalTwo = graph->AddChild(internalOne);
  vtkIdType a = graph->AddChild(internalTwo);
  graph->AddChild(internalTwo);
  graph->AddChild(internalOne);
  vtkIdType b = graph->AddChild(a);
  vtkIdType c = graph->AddChild(a);

  int numNodes = 8;
  vtkNew<vtkDoubleArray> weights;
  weights->SetNumberOfComponents(1);
  weights->SetName("weight");
  weights->SetNumberOfValues(numNodes-1);//the number of edges = number of nodes -1 for a tree
  weights->FillComponent(0, 0.0);

  // Create the names array
  vtkNew<vtkStringArray> names;
  names->SetNumberOfComponents(1);
  names->SetName("node name");
  names->SetNumberOfValues(numNodes);
  names->SetValue(0,"root");
  names->SetValue(5,"d");
  names->SetValue(3,"a");
  names->SetValue(6,"b");
  names->SetValue(7,"c");


  graph->GetEdgeData()->AddArray(weights.GetPointer());
  graph->GetVertexData()->AddArray(names.GetPointer());

  vtkNew<vtkTree> tree;
  tree->ShallowCopy(graph.GetPointer());

  int SUCCESS = 0;

  // subtest 1
  vtkNew<vtkSelection> sel;
  vtkNew<vtkSelectionNode> selNode;
  vtkNew<vtkIdTypeArray> selArr;
  selArr->InsertNextValue(a);
  selArr->InsertNextValue(b);
  selArr->InsertNextValue(c);
  selNode->SetContentType(vtkSelectionNode::INDICES);
  selNode->SetFieldType(vtkSelectionNode::VERTEX);
  selNode->SetSelectionList(selArr.GetPointer());
  selNode->GetProperties()->Set(vtkSelectionNode::INVERSE(), 1);
  sel->AddNode(selNode.GetPointer());

  vtkNew<vtkExtractSelectedTree> filter1;
  filter1->SetInputData(0,tree.GetPointer());
  filter1->SetInputData(1,sel.GetPointer());
  vtkTree * resultTree1 = filter1->GetOutput();
  filter1->Update();

  if (resultTree1->GetNumberOfVertices() == 5)
    {

    vtkDataSetAttributes * vertexData = resultTree1->GetVertexData();
    vtkDataSetAttributes * edgeData = resultTree1->GetEdgeData();
    if (vertexData->GetNumberOfTuples() != 5)
      {
      std::cerr << "vertex # =" << vertexData->GetNumberOfTuples() << std::endl;
      return EXIT_FAILURE;
      }
    else
      {
      vtkStringArray * nodename = vtkStringArray::SafeDownCast(vertexData->GetAbstractArray("node name"));
      vtkStdString n = nodename->GetValue(4);
      if (n.compare("d") != 0)
        {
        std::cerr <<"The node name should be \'d\', but appear to be: "<< n.c_str() << std::endl;
        return EXIT_FAILURE;
        }
      }

    if (edgeData->GetNumberOfTuples() != 4)
      {
      std::cerr<<"edge # ="<<edgeData->GetNumberOfTuples()<<std::endl;
      return EXIT_FAILURE;
      }
    SUCCESS++;
    }

  //subtest 2
  vtkNew<vtkExtractSelectedTree> filter2;
  selNode->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  filter2->SetInputData(0,tree.GetPointer());
  filter2->SetInputData(1,sel.GetPointer());
  vtkTree * resultTree2 = filter2->GetOutput();
  filter2->Update();

  if (resultTree2->GetNumberOfVertices() == 3)
    {
    SUCCESS++;
    }
  else
    {
    std::cerr<<"sub test 2: edge # ="<<resultTree2->GetNumberOfEdges()<<std::endl;
    std::cerr<<"vertex # ="<<resultTree2->GetNumberOfVertices()<<std::endl;
    return EXIT_FAILURE;
    }



  //sub test 3
  vtkNew<vtkExtractSelectedTree> filter3;
  vtkNew<vtkSelection> sel3;
  vtkNew<vtkSelectionNode> selEdge;
  vtkNew<vtkIdTypeArray> selArrEdge;
  selArrEdge->InsertNextValue(5);
  selArrEdge->InsertNextValue(6);
  selEdge->SetContentType(vtkSelectionNode::INDICES);
  selEdge->SetFieldType(vtkSelectionNode::EDGE);
  selEdge->SetSelectionList(selArrEdge.GetPointer());
  selEdge->GetProperties()->Set(vtkSelectionNode::INVERSE(), 0);
  sel3->AddNode(selEdge.GetPointer());

  filter3->SetInputData(0,tree.GetPointer());
  filter3->SetInputData(1,sel3.GetPointer());
  vtkTree * resultTree3 = filter3->GetOutput();
  filter3->Update();

  if (resultTree3->GetNumberOfVertices() == 3)
    {
    SUCCESS++;
    }
  else
    {
    std::cerr<<"sub test 3: edge # ="<<resultTree3->GetNumberOfEdges()<<std::endl;
    std::cerr<<"vertex # ="<<resultTree3->GetNumberOfVertices()<<std::endl;
    return EXIT_FAILURE;
    }

  if( SUCCESS == 3)
    {
    return EXIT_SUCCESS;
    }

  return EXIT_FAILURE;
}
