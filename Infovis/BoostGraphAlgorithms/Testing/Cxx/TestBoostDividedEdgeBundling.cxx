/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiagram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBoostDividedEdgeBundling.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkGraphItem.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkStringArray.h"
#include "vtkXMLTreeReader.h"
#include "vtkViewTheme.h"

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkContext2D.h"
#include "vtkContextInteractorStyle.h"
#include "vtkContextItem.h"
#include "vtkContextActor.h"
#include "vtkContextScene.h"
#include "vtkContextTransform.h"
#include "vtkNew.h"

#include "vtkRegressionTestImage.h"

//----------------------------------------------------------------------------
void BuildSampleGraph(vtkMutableDirectedGraph* graph)
{
  vtkNew<vtkPoints> points;

  graph->AddVertex();
  points->InsertNextPoint(20, 40, 0);
  graph->AddVertex();
  points->InsertNextPoint(20, 80, 0);
  graph->AddVertex();
  points->InsertNextPoint(20, 120, 0);
  graph->AddVertex();
  points->InsertNextPoint(20, 160, 0);
  graph->AddVertex();
  points->InsertNextPoint(380, 40, 0);
  graph->AddVertex();
  points->InsertNextPoint(380, 80, 0);
  graph->AddVertex();
  points->InsertNextPoint(380, 120, 0);
  graph->AddVertex();
  points->InsertNextPoint(380, 160, 0);
  graph->SetPoints(points.GetPointer());

  graph->AddEdge(0, 4);
  graph->AddEdge(0, 5);
  graph->AddEdge(1, 4);
  graph->AddEdge(1, 5);
  graph->AddEdge(2, 6);
  graph->AddEdge(2, 7);
  graph->AddEdge(3, 6);
  graph->AddEdge(3, 7);

  graph->AddEdge(4, 0);
  graph->AddEdge(5, 0);
  graph->AddEdge(6, 0);
}

//----------------------------------------------------------------------------
void BuildGraphMLGraph(vtkMutableDirectedGraph* graph, std::string file)
{
  vtkNew<vtkXMLTreeReader> reader;
  reader->SetFileName(file.c_str());
  reader->ReadCharDataOn();
  reader->Update();
  vtkTree *tree = reader->GetOutput();
  vtkStringArray *keyArr = vtkStringArray::SafeDownCast(
    tree->GetVertexData()->GetAbstractArray("key"));
  vtkStringArray *sourceArr = vtkStringArray::SafeDownCast(
    tree->GetVertexData()->GetAbstractArray("source"));
  vtkStringArray *targetArr = vtkStringArray::SafeDownCast(
    tree->GetVertexData()->GetAbstractArray("target"));
  vtkStringArray *contentArr = vtkStringArray::SafeDownCast(
    tree->GetVertexData()->GetAbstractArray(".chardata"));
  double x = 0.0;
  double y = 0.0;
  vtkIdType source = 0;
  vtkIdType target = 0;
  vtkNew<vtkPoints> points;
  graph->SetPoints(points.GetPointer());
  for (vtkIdType i = 0; i < tree->GetNumberOfVertices(); ++i)
    {
    vtkStdString k = keyArr->GetValue(i);
    if (k == "x")
      {
      x = vtkVariant(contentArr->GetValue(i)).ToDouble();
      }
    if (k == "y")
      {
      y = vtkVariant(contentArr->GetValue(i)).ToDouble();
      graph->AddVertex();
      points->InsertNextPoint(x, y, 0.0);
      }
    vtkStdString s = sourceArr->GetValue(i);
    if (s != "")
      {
      source = vtkVariant(s).ToInt();
      }
    vtkStdString t = targetArr->GetValue(i);
    if (t != "")
      {
      target = vtkVariant(t).ToInt();
      graph->AddEdge(source, target);
      }
    }
}

//----------------------------------------------------------------------------
class vtkBundledGraphItem : public vtkGraphItem
{
public:
  static vtkBundledGraphItem *New();
  vtkTypeMacro(vtkBundledGraphItem, vtkGraphItem);

protected:
  vtkBundledGraphItem() { }
  ~vtkBundledGraphItem() { }

  virtual vtkColor4ub EdgeColor(vtkIdType line, vtkIdType point);
  virtual float EdgeWidth(vtkIdType line, vtkIdType point);
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBundledGraphItem);

//----------------------------------------------------------------------------
vtkColor4ub vtkBundledGraphItem::EdgeColor(vtkIdType edgeIdx, vtkIdType pointIdx)
{
  float fraction = static_cast<float>(pointIdx) / (this->NumberOfEdgePoints(edgeIdx) - 1);
  return vtkColor4ub(fraction*255, 0, 255 - fraction*255, 255);
}

//----------------------------------------------------------------------------
float vtkBundledGraphItem::EdgeWidth(vtkIdType vtkNotUsed(lineIdx),
                                     vtkIdType vtkNotUsed(pointIdx))
{
  return 4.0f;
}

//----------------------------------------------------------------------------
int TestBoostDividedEdgeBundling(int argc, char* argv[])
{
  vtkNew<vtkMutableDirectedGraph> graph;
  vtkNew<vtkBoostDividedEdgeBundling> bundle;

  BuildSampleGraph(graph.GetPointer());
  //BuildGraphMLGraph(graph.GetPointer(), "airlines_flipped.graphml");

  bundle->SetInputData(graph.GetPointer());
  bundle->Update();

  vtkDirectedGraph *output = bundle->GetOutput();

  vtkNew<vtkContextActor> actor;

  vtkNew<vtkBundledGraphItem> graphItem;
  graphItem->SetGraph(output);

  vtkNew<vtkContextTransform> trans;
  trans->SetInteractive(true);
  trans->AddItem(graphItem.GetPointer());
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

