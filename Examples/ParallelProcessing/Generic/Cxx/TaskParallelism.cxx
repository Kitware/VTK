// This example demonstrates now to write a task parallel application
// with VTK. It creates two different pipelines and assigns each to
// one processor. These pipelines are:
// 1. rtSource -> contour            -> probe
//             \                     /
//              -> gradient magnitude
// 2. rtSource -> gradient -> shrink -> glyph3D

#include "TaskParallelism.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTreeComposite.h"

// This function sets up properties common to both processes
// and executes the task corresponding to the current process
void process(vtkMultiProcessController* controller, void* arg)
{
  taskFunction task;
  int myId = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();

  // Chose the appropriate task (see task1.cxx and task2.cxx)
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

  // Create the render objects
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->SetSize( WINDOW_WIDTH, WINDOW_HEIGHT );

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  iren->Initialize();

  // This class allows all processes to composite their images.
  // The root process then displays it in it's render window.
  vtkTreeComposite* tc = vtkTreeComposite::New();
  tc->SetRenderWindow(renWin);

// Generate the pipeline see task1.cxx and task2.cxx)
  vtkPolyDataMapper* mapper = (*task)(renWin, EXTENT, cam);
  
  // Only the root process will have an active interactor. All
  // the other render windows will be slaved to the root.
  iren->Start();

  // Clean-up
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
  
  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkMultiProcessController* controller = vtkMultiProcessController::New();
  controller->Initialize(&argc, &argv);

  // When using MPI, the number of processes is determined
  // by the external program which launches this application.
  // However, when using threads, we need to set it ourselves.
  if (controller->IsA("vtkThreadedController"))
    {
    // Set the number of processes to 2 for this example.
    controller->SetNumberOfProcesses(2);
    } 
  int myId = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();

  if (numProcs != 2)
    {
    cerr << "This example requires two processes." << endl;
    controller->Finalize();
    controller->Delete();
    return 1;
    }


  // Execute the function named "process" on both processes
  controller->SetSingleMethod(process, 0);
  controller->SingleMethodExecute();
  
  // Clean-up and exit
  controller->Finalize();
  controller->Delete();
  
  return 0;
}









