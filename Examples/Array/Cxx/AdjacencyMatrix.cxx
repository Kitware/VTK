#include <vtkAdjacencyMatrixToEdgeTable.h>
#include <vtkArrayPrint.h>
#include <vtkDenseArray.h>
#include <vtkDiagonalMatrixSource.h>
#include <vtkRenderWindow.h>
#include <vtkGraphLayoutView.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkTableToGraph.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>

int main(int argc, char* argv[])
{
  vtkSmartPointer<vtkDiagonalMatrixSource> source = vtkSmartPointer<vtkDiagonalMatrixSource>::New();
  source->SetExtents(10);
  source->SetDiagonal(0);
  source->SetSuperDiagonal(1);
  source->SetSubDiagonal(2);
  source->Update();

  cout << "adjacency matrix:\n";
  vtkPrintMatrixFormat(cout, vtkDenseArray<double>::SafeDownCast(source->GetOutput()->GetArray()));
  cout << "\n";

  vtkSmartPointer<vtkAdjacencyMatrixToEdgeTable> edges = vtkSmartPointer<vtkAdjacencyMatrixToEdgeTable>::New();
  edges->SetInputConnection(source->GetOutputPort());

  vtkSmartPointer<vtkTableToGraph> graph = vtkSmartPointer<vtkTableToGraph>::New();
  graph->SetInputConnection(edges->GetOutputPort());
  graph->AddLinkVertex("rows", "stuff", false);
  graph->AddLinkVertex("columns", "stuff", false);
  graph->AddLinkEdge("rows", "columns");

  vtkSmartPointer<vtkGraphLayoutView> view = vtkSmartPointer<vtkGraphLayoutView>::New();
  view->AddRepresentationFromInputConnection(graph->GetOutputPort());
  view->EdgeLabelVisibilityOn();
  view->SetEdgeLabelArrayName("value");

  vtkSmartPointer<vtkRenderWindow> window = vtkSmartPointer<vtkRenderWindow>::New();
  window->SetSize(600, 600);
  view->SetupRenderWindow(window);
  view->GetRenderer()->ResetCamera();
  window->GetInteractor()->Start();
 
  return 0;
}

