// this program test the functionality of the vtkMPIController
// Send/Receive integer arrays,
// Send/Receive vtkDataObjects,
// Remote method invocation.

#include "mpi.h"
#include "vtkMPIController.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkElevationFilter.h"
#include "vtkRenderWindowInteractor.h"



void callback1(void *arg, int id)
{
  cerr << "RMI triggered by " << id << " executed call back 1\n";
}

void callback2(void *arg, int id)
{
  cerr << "RMI triggered by " << id << " executed call back 2\n";
}

void callback3(void *arg, int id)
{
  char *str = (char*)(arg);
  cerr << "RMI triggered by " << id << " executed call back 3: " 
       << str << "\n";
}



void main( int argc, char *argv[] )
{
  char a;
  int numprocs, myid;
  int id0 = 0;
  int id1 = 1;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  cerr << "process: " << myid << " of " << numprocs << endl;
  
  // setup the pipeline in process 1
  if (myid == id1) {
    vtkConeSource *cone = vtkConeSource::New();
    vtkElevationFilter *elev = vtkElevationFilter::New();
    vtkMPIController *controller;
    int message = 12345;
    char *arg;
    
    controller = vtkMPIController::RegisterAndGetGlobalController(NULL);

    // first just send an integer to the other process.
    controller->Send(&message, 1, id0, 100);
    
    // now try to send some polydata
    cone->SetResolution(8);
    elev->SetInput(cone->GetOutput());
    elev->Update();
    controller->Send(elev->GetOutput(), id0, 200);
    
    // last, set up a RMI call backs
    controller->AddRMI(callback1, NULL, 301);
    controller->AddRMI(callback2, NULL, 302);
    arg = new char[20];
    strcpy(arg, "Hello World!");
    controller->AddRMI(callback3, (void*)(arg), 303);
    
    // wait for the call back to execute.
    controller->ProcessRMIs();
    
    cone->SetResolution(8);
    elev->SetInput(cone->GetOutput());

    controller->UnRegister(NULL);
    cone->Delete();
    elev->Delete();
    delete [] arg;
  }


  // set up the renderer in process 0
  if (myid == id0) 
    {
    vtkPolyData *data = vtkPolyData::New();
    vtkRenderer *ren = vtkRenderer::New();
    vtkRenderWindow *renWindow = vtkRenderWindow::New();
    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
    int message;
    vtkMPIController *controller;
    
    controller = vtkMPIController::RegisterAndGetGlobalController(NULL);
    
    // first receive the integer message.
    controller->Receive(&message, 1, id1, 100);
    
    cerr << "received message " << message << endl;
    
    // now receive the poly data object
    controller->Receive(data, id1, 200);
    
    // before we display this polydata, fire off some RMIs
    controller->TriggerRMI(id1, 303);
    controller->TriggerRMI(id1, 302);
    controller->TriggerRMI(id1, 301);
    
    
    
    renWindow->AddRenderer(ren);
    iren->SetRenderWindow(renWindow);
    renWindow->SetSize( 300, 300 );
    
    coneMapper->SetInput(data);
    vtkActor *coneActor = vtkActor::New();
    coneActor->SetMapper(coneMapper);
    
    // assign our actor to the renderer
    ren->AddActor(coneActor);
    
    // draw the resulting scene
    renWindow->Render();
    
    //  Begin mouse interaction
    iren->Start();
    
    // Clean up
    ren->Delete();
    renWindow->Delete();
    iren->Delete();
    coneMapper->Delete();
    coneActor->Delete();
  }

  cerr << myid << " waiting at barrier\n";
  MPI_Barrier (MPI_COMM_WORLD);
  cerr << myid << " past barrier\n";
  MPI_Finalize();
  return(0);
}


