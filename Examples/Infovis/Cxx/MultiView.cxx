/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MultiView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example...
//

#include "vtkAnnotationLink.h"
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraphLayoutView.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkRandomGraphSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStringArray.h"
#include "vtkTree.h"
#include "vtkViewTheme.h"

#include <vector>

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

  void Execute(vtkObject*, unsigned long, void*) VTK_OVERRIDE
  {
    for (unsigned int i = 0; i < this->Views.size(); i++)
    {
      this->Views[i]->Update();
    }
  }
private:
  ViewUpdater() { }
  ~ViewUpdater() VTK_OVERRIDE { }
  std::vector<vtkView*> Views;
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
  bool validTree = tree->CheckedShallowCopy(graph);
  if (!validTree)
  {
    std::cout << "ERROR: Invalid tree" << std::endl;
    graph->Delete();
    labels->Delete();
    tree->Delete();
    return EXIT_FAILURE;
  }
  vtkGraphLayoutView* view = vtkGraphLayoutView::New();
  vtkDataRepresentation* rep =
    view->SetRepresentationFromInput(tree);
  vtkViewTheme* theme = vtkViewTheme::CreateMellowTheme();
  view->ApplyViewTheme(theme);
  view->SetVertexColorArrayName("VertexDegree");
  view->SetColorVertices(true);
  view->SetVertexLabelArrayName("Label");
  view->SetVertexLabelVisibility(true);

  vtkGraphLayoutView* view2 = vtkGraphLayoutView::New();
  vtkDataRepresentation* rep2 =
    view2->SetRepresentationFromInput(tree);
  view2->SetVertexLabelArrayName("Label");
  view2->SetVertexLabelVisibility(true);

  vtkAnnotationLink* link = vtkAnnotationLink::New();
  rep->SetAnnotationLink(link);
  rep2->SetAnnotationLink(link);

  ViewUpdater* update = ViewUpdater::New();
  update->AddView(view);
  update->AddView(view2);

  view->ResetCamera();
  view2->ResetCamera();
  view->Render();
  view2->Render();
  view->GetInteractor()->Start();

  graph->Delete();
  labels->Delete();
  tree->Delete();
  view->Delete();
  theme->Delete();
  view2->Delete();
  link->Delete();
  update->Delete();

  return EXIT_SUCCESS;
}

