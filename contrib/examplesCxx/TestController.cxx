// this program test the functionality of the vtkMultiProcessController
// Send/Receive integer arrays,
// Send/Receive vtkDataObjects,
// Remote method invocation.


#include "vtkMultiProcessController.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkElevationFilter.h"
#include "vtkRenderWindowInteractor.h"



#define MESSAGE1 12345
#define MESSAGE2 -9999


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



VTK_THREAD_RETURN_TYPE process_a( void *vtkNotUsed(proc_arg) )
{
  int myid, otherid;
  char a;
  vtkConeSource *cone = vtkConeSource::New();
  vtkElevationFilter *elev = vtkElevationFilter::New();
  vtkMultiProcessController *controller;
  int message = MESSAGE1;
  char *arg;
  
  
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);
  myid = controller->GetLocalProcessId();
  if (myid == 0)
    {
    otherid = 1;
    }
  else
    {
    otherid = 0;
    }
  
  
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);
  
  // first just send an integer to the other process.
  message = MESSAGE1;
  controller->Send(&message, 1, otherid, 100);

  message = MESSAGE2;
  controller->Send(&message, 1, otherid, 100);

  // now try to send some polydata
  cone->SetResolution(8);
  elev->SetInput(cone->GetOutput());
  elev->Update();
  controller->Send(elev->GetOutput(), otherid, 200);
  
  // last, set up a RMI call backs
  controller->AddRMI(callback1, NULL, 301);
  controller->AddRMI(callback2, NULL, 302);
  arg = new char[20];
  strcpy(arg, "Hello World!");
  controller->AddRMI(callback3, (void*)(arg), 303);
  
  // Wait for the call back to execute.
  // This call will not return.
  controller->ProcessRMIs();
  
  controller->UnRegister(NULL);
  cone->Delete();
  elev->Delete();
  delete [] arg;
  
  return VTK_THREAD_RETURN_VALUE;
}


VTK_THREAD_RETURN_TYPE process_b( void *vtkNotUsed(proc_arg) )
{
  int myid, otherid;
  char a;
  vtkPolyData *data = vtkPolyData::New();
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWindow = vtkRenderWindow::New();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
  int message;
  vtkMultiProcessController *controller;
    
  
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);
  myid = controller->GetLocalProcessId();
  if (myid == 0)
    {
    otherid = 1;
    }
  else
    {
    otherid = 0;
    }

  putenv("DISPLAY=:0.0");
  
  // first receive the integer message.
  controller->Receive(&message, 1, otherid, 100);  
  cerr << "received message " << message
       << " should be " << MESSAGE1 << endl;

  controller->Receive(&message, 1, otherid, 100);  
  cerr << "received message " << message 
       << " should be " << MESSAGE2 << endl;
  
  // now receive the poly data object
  controller->Receive(data, otherid, 200);
  
  // before we display this polydata, fire off some RMIs
  controller->TriggerRMI(otherid, 303);
  controller->TriggerRMI(otherid, 302);
  controller->TriggerRMI(otherid, 301);
  
  cerr << "Test 1\n";
  
  renWindow->AddRenderer(ren);
  iren->SetRenderWindow(renWindow);
  renWindow->SetSize( 300, 300 );
  
  coneMapper->SetInput(data);
  vtkActor *coneActor = vtkActor::New();
  coneActor->SetMapper(coneMapper);
  
  // assign our actor to the renderer
  ren->AddActor(coneActor);
  
  cerr << "Test 2\n";
  // draw the resulting scene
  renWindow->Render();
  cerr << "Test 3\n";
  
  //  Begin mouse interaction
  iren->Start();
  
  // Clean up
  ren->Delete();
  renWindow->Delete();
  iren->Delete();
  coneMapper->Delete();
  coneActor->Delete();

  return VTK_THREAD_RETURN_VALUE;
}


void main( int argc, char *argv[] )
{
  vtkMultiProcessController *controller;
  int myid;
  
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);

  controller->Initialize(argc, argv);

  
  controller->SetNumberOfProcesses(2);
  controller->SetMultipleMethod(0, process_a, NULL);
  controller->SetMultipleMethod(1, process_b, NULL);
  controller->MultipleMethodExecute();

  controller->UnRegister(NULL);
}


