/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDendrogramOnly.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTreeHeatmapItem.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkTree.h"

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkContextInteractorStyle.h"
#include "vtkContextActor.h"
#include "vtkContextScene.h"
#include "vtkContextTransform.h"
#include "vtkNew.h"

#include "vtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestDendrogramOnly(int argc, char* argv[])
{
  vtkNew<vtkMutableDirectedGraph> graph;
  vtkIdType root = graph->AddVertex();
  vtkIdType internalOne = graph->AddChild(root);
  vtkIdType internalTwo = graph->AddChild(internalOne);
  vtkIdType a = graph->AddChild(internalTwo);
  vtkIdType b = graph->AddChild(internalTwo);
  vtkIdType c = graph->AddChild(internalOne);

  vtkNew<vtkDoubleArray> weights;
  weights->SetNumberOfTuples(5);
  weights->SetValue(graph->GetEdgeId(root, internalOne), 1.0f);
  weights->SetValue(graph->GetEdgeId(internalOne, internalTwo), 2.0f);
  weights->SetValue(graph->GetEdgeId(internalTwo, a), 1.0f);
  weights->SetValue(graph->GetEdgeId(internalTwo, b), 1.0f);
  weights->SetValue(graph->GetEdgeId(internalOne, c), 3.0f);

  weights->SetName("weight");
  graph->GetEdgeData()->AddArray(weights.GetPointer());

  vtkNew<vtkStringArray> names;
  names->SetNumberOfTuples(6);
  names->SetValue(a, "a");
  names->SetValue(b, "b");
  names->SetValue(c, "c");

  names->SetName("node name");
  graph->GetVertexData()->AddArray(names.GetPointer());

  vtkNew<vtkDoubleArray> nodeWeights;
  nodeWeights->SetNumberOfTuples(6);
  nodeWeights->SetValue(root, 0.0f);
  nodeWeights->SetValue(internalOne, 1.0f);
  nodeWeights->SetValue(internalTwo, 3.0f);
  nodeWeights->SetValue(a, 4.0f);
  nodeWeights->SetValue(b, 4.0f);
  nodeWeights->SetValue(c, 4.0f);
  nodeWeights->SetName("node weight");
  graph->GetVertexData()->AddArray(nodeWeights.GetPointer());

  vtkNew<vtkContextActor> actor;

  vtkNew<vtkTree> tree;
  tree->ShallowCopy(graph.GetPointer());

  vtkNew<vtkTreeHeatmapItem> treeItem;
  treeItem->SetTree(tree.GetPointer());

  vtkNew<vtkContextTransform> trans;
  trans->SetInteractive(true);
  trans->AddItem(treeItem.GetPointer());
  actor->GetScene()->AddItem(trans.GetPointer());

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(1.0, 1.0, 1.0);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 200);
  renderWindow->AddRenderer(renderer.GetPointer());
  renderer->AddActor(actor.GetPointer());

  vtkNew<vtkContextInteractorStyle> interactorStyle;
  interactorStyle->SetScene(actor->GetScene());

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetInteractorStyle(interactorStyle.GetPointer());
  interactor->SetRenderWindow(renderWindow.GetPointer());
  renderWindow->SetMultiSamples(0);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    renderWindow->Render();
    interactor->Start();
    retVal = vtkRegressionTester::PASSED;
    }
  return !retVal;
}
