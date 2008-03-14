
#include "vtkGraphLayoutView.h"
#include "vtkRandomGraphSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkViewTheme.h"

int main(int, char*[])
{
  vtkRandomGraphSource* source = vtkRandomGraphSource::New();
  
  vtkGraphLayoutView* view = vtkGraphLayoutView::New();
  view->AddRepresentationFromInputConnection(
    source->GetOutputPort());
  
  vtkViewTheme* theme = vtkViewTheme::CreateMellowTheme();
  view->ApplyViewTheme(theme);
  theme->Delete();
  view->SetVertexColorArrayName("VertexDegree");
  view->SetColorVertices(true);
  view->SetVertexLabelArrayName("VertexDegree");
  view->SetVertexLabelVisibility(true);

  vtkRenderWindow* window = vtkRenderWindow::New();
  view->SetupRenderWindow(window);
  window->GetInteractor()->Start();
  
  source->Delete();
  view->Delete();
  window->Delete();
  
  return 0;
}
