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


void callback1(void *loaclArg, void *remoteArg, int remoteArgLength, int id)
{
  cout << "RMI triggered by " << id << " executed call back 1\n";
}

void callback2(void *localArg, void *remoteArg, int remoteArgLength, int id)
{
  cout << "RMI triggered by " << id << " executed call back 2\n";
}

void callback3(void *localArg, void *remoteArg, int remoteArgLength, int id)
{
  char *str = (char*)(localArg);
  char *str2 = (char*)(remoteArg);
  cout << "RMI triggered by " << id << "(" << str2 << ") executed call back 3: " 
       << str << "\n";
}



void process_a(vtkMultiProcessController *controller,
	       void *processArg)
{
  int otherId;
  vtkConeSource *cone = vtkConeSource::New();
  vtkElevationFilter *elev = vtkElevationFilter::New();
  int message = MESSAGE1;
  char *arg;
  int myId;

  // If You do not have a pointer to the controller, 
  // you can get one with this static method.
  //controller = vtkMultiProcessController::GetGlobalController();
  
  myId = controller->GetLocalProcessId();  
  
  if (myId == 0)
    {
    otherId = 1;
    }
  else
    {
    otherId = 0;
    }
  
  
  // first just send an integer to the other process.
  message = MESSAGE1;
  controller->Send(&message, 1, otherId, 100);

  message = MESSAGE2;
  controller->Send(&message, 1, otherId, 100);

  // now try to send some polydata
  cone->SetResolution(8);
  elev->SetInput(cone->GetOutput());
  elev->Update();
  controller->Send(elev->GetOutput(), otherId, 200);
  
  // last, set up a RMI call backs
  controller->AddRMI(callback1, NULL, 301);
  controller->AddRMI(callback2, NULL, 302);
  arg = new char[20];
  strcpy(arg, "Fine, Thank you.");
  controller->AddRMI(callback3, (void*)(arg), 303);
  
  // Wait for the call back to execute.
  // This call will not return.
  controller->ProcessRMIs();

  cone->Delete();
  elev->Delete();
  delete [] arg;
}


void process_b(vtkMultiProcessController *controller, void *arg)
{
  int myId, otherId;
  vtkPolyData *data = vtkPolyData::New();
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWindow = vtkRenderWindow::New();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
  int message;
    
  // If You do not have a pointer to the controller, 
  // you can get one with this static method.
  //controller = vtkMultiProcessController::GetGlobalController();
  
  myId = controller->GetLocalProcessId();
  
  if (myId == 0)
    {
    otherId = 1;
    }
  else
    {
    otherId = 0;
    }

  // first receive the integer message.
  controller->Receive(&message, 1, otherId, 100);  
  cout << "received message " << message
       << " should be " << MESSAGE1 << endl;

  controller->Receive(&message, 1, otherId, 100);  
  cout << "received message " << message 
       << " should be " << MESSAGE2 << endl;
  
  // now receive the poly data object
  controller->Receive(data, otherId, 200);
  
  // before we display this polydata, fire off some RMIs
  controller->TriggerRMI(otherId, 301);
  controller->TriggerRMI(otherId, 302);

  char argg[13];
  strcpy(argg, "How are you?");
  controller->TriggerRMI(otherId, argg, 303);
  controller->TriggerRMI(otherId,vtkMultiProcessController::BREAK_RMI_TAG);
  
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
  // iren->Start();
  
  // Clean up
  ren->Delete();
  renWindow->Delete();
  iren->Delete();
  coneMapper->Delete();
  coneActor->Delete();
}

int main( int argc, char *argv[] )
{
  vtkMultiProcessController *controller;
  
  controller = vtkMultiProcessController::New();
  controller->Initialize(&argc, &argv);

  controller->SetNumberOfProcesses(2);
  controller->SetMultipleMethod(0, process_a, NULL);
  controller->SetMultipleMethod(1, process_b, NULL);
  controller->MultipleMethodExecute();

  vtkGenericWarningMacro("Testing the output window.");
  controller->Finalize();
  controller->Delete();
  vtkGenericWarningMacro("Testing the output window.");
  
  return 0;
}








