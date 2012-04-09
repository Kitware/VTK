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
#include <vtkViewTheme.h>

int main(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkDiagonalMatrixSource> source = vtkSmartPointer<vtkDiagonalMatrixSource>::New();
  source->SetExtents(10);
  source->SetDiagonal(0);
  source->SetSuperDiagonal(1);
  source->SetSubDiagonal(2);
  source->Update();

  cout << "adjacency matrix:\n";
  vtkPrintMatrixFormat(cout, vtkDenseArray<double>::SafeDownCast(source->GetOutput()->GetArray(0)));
  cout << "\n";

  vtkSmartPointer<vtkAdjacencyMatrixToEdgeTable> edges = vtkSmartPointer<vtkAdjacencyMatrixToEdgeTable>::New();
  edges->SetInputConnection(source->GetOutputPort());

  vtkSmartPointer<vtkTableToGraph> graph = vtkSmartPointer<vtkTableToGraph>::New();
  graph->SetInputConnection(edges->GetOutputPort());
  graph->AddLinkVertex("rows", "stuff", false);
  graph->AddLinkVertex("columns", "stuff", false);
  graph->AddLinkEdge("rows", "columns");

  vtkSmartPointer<vtkViewTheme> theme;
  theme.TakeReference(vtkViewTheme::CreateMellowTheme());
  theme->SetLineWidth(5);
  theme->SetCellOpacity(0.9);
  theme->SetCellAlphaRange(0.5,0.5);
  theme->SetPointSize(10);
  theme->SetSelectedCellColor(1,0,1);
  theme->SetSelectedPointColor(1,0,1);

  vtkSmartPointer<vtkGraphLayoutView> view = vtkSmartPointer<vtkGraphLayoutView>::New();
  view->AddRepresentationFromInputConnection(graph->GetOutputPort());
  view->EdgeLabelVisibilityOn();
  view->SetEdgeLabelArrayName("value");
  view->ApplyViewTheme(theme);
  view->SetVertexLabelFontSize(20);
  view->SetEdgeLabelFontSize(18);
  view->VertexLabelVisibilityOn();

  view->GetRenderWindow()->SetSize(600, 600);
  view->ResetCamera();
  view->GetInteractor()->Start();

  return 0;
}

