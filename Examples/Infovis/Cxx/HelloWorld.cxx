
#include "vtkGraphLayoutView.h"
#include "vtkRandomGraphSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

int main(int argc, char* argv[])
{
  vtkRandomGraphSource* source = vtkRandomGraphSource::New();
  
  vtkGraphLayoutView* view = vtkGraphLayoutView::New();
  vtkDataRepresentation* rep =
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
