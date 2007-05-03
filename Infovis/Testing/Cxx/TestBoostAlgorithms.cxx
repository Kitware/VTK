
#include "vtkActor.h"
#include "vtkBoostBiconnectedComponents.h"
#include "vtkBoostBrandesCentrality.h"
#include "vtkBoostBreadthFirstSearch.h"
#include "vtkBoostConnectedComponents.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkGraph.h"
#include "vtkGraphToPolyData.h"
#include "vtkGraphWriter.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

void RenderGraph(vtkRenderer* ren, vtkGraphAlgorithm* alg, 
  double xoffset, double yoffset, 
  const char* vertColorArray, double vertMin, double vertMax, 
  const char* edgeColorArray, double edgeMin, double edgeMax)
{
  VTK_CREATE(vtkGraphToPolyData, graphToPoly);
  graphToPoly->SetInputConnection(alg->GetOutputPort());

  VTK_CREATE(vtkGlyphSource2D, glyph);
  glyph->SetGlyphTypeToVertex();
  VTK_CREATE(vtkGlyph3D, vertexGlyph);
  vertexGlyph->SetInputConnection(0, graphToPoly->GetOutputPort());
  vertexGlyph->SetInputConnection(1, glyph->GetOutputPort());
  VTK_CREATE(vtkPolyDataMapper, vertexMapper);
  vertexMapper->SetInputConnection(vertexGlyph->GetOutputPort());
  vertexMapper->SetScalarModeToUsePointFieldData();
  if (vertColorArray)
    {
    vertexMapper->SelectColorArray(vertColorArray);
    vertexMapper->SetScalarRange(vertMin, vertMax);
    }
  VTK_CREATE(vtkActor, vertexActor);
  vertexActor->SetMapper(vertexMapper);
  vertexActor->GetProperty()->SetPointSize(10.0);
  vertexActor->SetPosition(xoffset, yoffset, 0.001);

  VTK_CREATE(vtkPolyDataMapper, edgeMapper);
  edgeMapper->SetInputConnection(graphToPoly->GetOutputPort());
  edgeMapper->SetScalarModeToUseCellFieldData();
  if (edgeColorArray)
    {
    edgeMapper->SelectColorArray(edgeColorArray);
    edgeMapper->SetScalarRange(edgeMin, edgeMax);
    }
  VTK_CREATE(vtkActor, edgeActor);
  edgeActor->SetMapper(edgeMapper);
  edgeActor->SetPosition(xoffset, yoffset, 0);

  ren->AddActor(vertexActor);
  ren->AddActor(edgeActor);
}

int TestBoostAlgorithms(int argc, char* argv[])
{
  // Create the test graph
  VTK_CREATE(vtkGraph, g);
  g->SetDirected(false);

  VTK_CREATE(vtkPoints, pts);
  g->AddVertex();
  pts->InsertNextPoint(0, 1, 0);
  g->AddVertex();
  pts->InsertNextPoint(0.5, 1, 0);
  g->AddVertex();
  pts->InsertNextPoint(0.25, 0.5, 0);
  g->AddVertex();
  pts->InsertNextPoint(0, 0, 0);
  g->AddVertex();
  pts->InsertNextPoint(0.5, 0, 0);
  g->AddVertex();
  pts->InsertNextPoint(1, 0, 0);
  g->AddVertex();
  pts->InsertNextPoint(0.75, 0.5, 0);
  g->SetPoints(pts);

  g->AddEdge(0, 1);
  g->AddEdge(0, 2);
  g->AddEdge(1, 2);
  g->AddEdge(2, 3);
  g->AddEdge(2, 4);
  g->AddEdge(3, 4);

  VTK_CREATE(vtkRenderer, ren);

  // Test biconnected components
  VTK_CREATE(vtkBoostBiconnectedComponents, biconn);
  biconn->SetInput(g);
  RenderGraph(ren, biconn, 0, 0, "biconnected component", -1, 3, "biconnected component", -1, 3);

  // Test breadth first search
  VTK_CREATE(vtkBoostBreadthFirstSearch, bfs);
  bfs->SetInput(g);
  RenderGraph(ren, bfs, 2, 0, "BFS", 0, 3, NULL, 0, 0);

  // Test centrality
  VTK_CREATE(vtkBoostBrandesCentrality, centrality);
  centrality->SetInput(g);
  RenderGraph(ren, centrality, 0, 2, "centrality", 0, 1, NULL, 0, 0);

  // Test connected components
  VTK_CREATE(vtkBoostConnectedComponents, comp);
  comp->SetInput(g);
  RenderGraph(ren, comp, 2, 2, "component", 0, 2, NULL, 0, 0);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    win->Render();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
    }

  return !retVal;
}

