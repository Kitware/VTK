#include "TaskParallelism.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTreeComposite.h"

void process(vtkMultiProcessController* controller, void* arg)
{
  taskFunction task;
  int myId = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();

  if ( myId == 0 )
    {
    task = task1;
    }
  else
    {
    task = task2;
    }


// Setup camera
  vtkCamera* cam = vtkCamera::New();
  cam->SetPosition( -0.6105, 1.467, -6.879 );
  cam->SetFocalPoint( -0.0617558, 0.127043, 0 );
  cam->SetViewUp( -0.02, 0.98, 0.193 );
  cam->SetClippingRange( 3.36, 11.67);
  cam->Dolly(0.8);

  vtkRenderWindow* renWin = vtkRenderWindow::New();
// Generate the pipeline
  vtkPolyDataMapper* mapper = (*task)(renWin, EXTENT, cam);

  renWin->SetSize( WINDOW_WIDTH, WINDOW_HEIGHT );
  
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  iren->Initialize();

  vtkTreeComposite* tc = vtkTreeComposite::New();
  tc->SetRenderWindow(renWin);

  iren->Start();
  cout << "foo" << endl;
  iren->Delete();

  if (mapper)
    {
    mapper->Delete();
    }
  renWin->Delete();
  cam->Delete();

}


int main( int argc, char* argv[] )
{
  taskFunction task[2];

  vtkMultiProcessController* controller = vtkMultiProcessController::New();
  controller->Initialize(&argc, &argv);
  int myId = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();

  if (numProcs != 2)
    {
    cerr << "This example requires two processes." << endl;
    controller->Finalize();
    controller->Delete();
    return 1;
    }


  controller->SetSingleMethod(process, 0);

  // Execute
  controller->SingleMethodExecute();
  
  controller->Finalize();
  controller->Delete();
  
  return 0;
}









