
#include "vtkGraphLayoutView.h"
#include "vtkRandomGraphSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

int main(int, char*[])
{
  vtkRandomGraphSource* source = vtkRandomGraphSource::New();
  
  vtkGraphLayoutView* view = vtkGraphLayoutView::New();
  view->AddRepresentationFromInputConnection(
    source->GetOutputPort());
  
  vtkRenderWindow* window = vtkRenderWindow::New();
  view->SetupRenderWindow(window);
  window->GetInteractor()->Start();
  
  source->Delete();
  view->Delete();
  window->Delete();
  
  return 0;
}
