
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraphLayoutView.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkRandomGraphSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelectionLink.h"
#include "vtkStringArray.h"
#include "vtkTree.h"
#include "vtkTreeLayoutView.h"
#include "vtkViewTheme.h"

#include <vtksys/stl/vector>

class ViewUpdater : public vtkCommand
{
public:
  static ViewUpdater* New()
  { return new ViewUpdater; }
  
  void AddView(vtkView* view)
  {
    this->Views.push_back(view);
    view->AddObserver(vtkCommand::SelectionChangedEvent, this);
  }
  
  virtual void Execute(vtkObject*, unsigned long, void*)
  {
    for (unsigned int i = 0; i < this->Views.size(); i++)
      {
      this->Views[i]->Update();
      }
  }
private:
  ViewUpdater() { }  
  ~ViewUpdater() { }
  vtksys_stl::vector<vtkView*> Views;
};

int main(int, char*[])
{
  vtkMutableDirectedGraph* graph = vtkMutableDirectedGraph::New();
  vtkIdType a = graph->AddVertex();
  vtkIdType b = graph->AddChild(a);
  vtkIdType c = graph->AddChild(a);
  vtkIdType d = graph->AddChild(b);
  vtkIdType e = graph->AddChild(c);
  vtkIdType f = graph->AddChild(c);
  
  vtkStringArray* labels = vtkStringArray::New();
  labels->SetName("Label");
  labels->InsertValue(a, "a");
  labels->InsertValue(b, "b");
  labels->InsertValue(c, "c");
  labels->InsertValue(d, "d");
  labels->InsertValue(e, "e");
  labels->InsertValue(f, "f");
  graph->GetVertexData()->AddArray(labels);
  
  vtkTree* tree = vtkTree::New();
  tree->CheckedShallowCopy(graph);
  
  vtkGraphLayoutView* view = vtkGraphLayoutView::New();
  vtkDataRepresentation* rep =
    view->AddRepresentationFromInput(tree);  
  vtkViewTheme* theme = vtkViewTheme::CreateMellowTheme();
  view->ApplyViewTheme(theme);
  view->SetVertexColorArrayName("VertexDegree");
  view->SetColorVertices(true);
  view->SetVertexLabelArrayName("Label");
  view->SetVertexLabelVisibility(true);
  vtkRenderWindow* window = vtkRenderWindow::New();
  view->SetupRenderWindow(window);

  vtkTreeLayoutView* view2 = vtkTreeLayoutView::New();
  vtkDataRepresentation* rep2 =
    view2->AddRepresentationFromInput(tree);
  view2->SetLabelArrayName("Label");
  view2->SetLabelVisibility(true);
  vtkRenderWindow* window2 = vtkRenderWindow::New();
  view2->SetupRenderWindow(window2);
  
  vtkSelectionLink* link = vtkSelectionLink::New();
  rep->SetSelectionLink(link);
  rep2->SetSelectionLink(link);
  
  ViewUpdater* update = ViewUpdater::New();
  update->AddView(view);
  update->AddView(view2);
  
  window->GetInteractor()->Start();
  
  graph->Delete();
  labels->Delete();
  tree->Delete();
  view->Delete();
  theme->Delete();
  window->Delete();
  view2->Delete();
  window2->Delete();
  link->Delete();
  update->Delete();
  
  return 0;
}

