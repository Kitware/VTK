// This program test the ports by setting up a simple pipeline.

#include "mpi.h"
#include "vtkOutputPort.h"
#include "vtkInputPort.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkElevationFilter.h"
#include "vtkRenderWindowInteractor.h"


VTK_THREAD_RETURN_TYPE process_a( void *vtkNotUsed(arg) )
{
  vtkMultiProcessController *controller;
  vtkConeSource *cone = vtkConeSource::New();
  vtkElevationFilter *elev = vtkElevationFilter::New();
  vtkOutputPort *upStreamPort = vtkOutputPort::New();
  int myid;
  
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);
  myid = controller->GetLocalProcessId();
  
  // Set up the pipeline source.
  cone->SetResolution(8);
  elev->SetInput(cone->GetOutput());
  upStreamPort->SetInput(elev->GetPolyDataOutput());
  upStreamPort->SetTag(999);
  
  // wait for the call back to execute.
  upStreamPort->WaitForUpdate();
  
  cone->Delete();
  elev->Delete();
  upStreamPort->Delete();

  return VTK_THREAD_RETURN_VALUE;
}


VTK_THREAD_RETURN_TYPE process_b( void *vtkNotUsed(arg) )
{
  vtkMultiProcessController *controller;
  int myid, otherid;
  
  putenv("DISPLAY=:0.0");
  
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

  vtkInputPort *downStreamPort = vtkInputPort::New();
  downStreamPort->SetRemoteProcessId(otherid);
  downStreamPort->SetTag(999);
  downStreamPort->GetPolyDataOutput()->SetUpdateExtent(0, 2);
  downStreamPort->Update();  

  vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
  coneMapper->SetInput(downStreamPort->GetPolyDataOutput());

  vtkActor *coneActor = vtkActor::New();
  coneActor->SetMapper(coneMapper);
  
  vtkRenderer *ren = vtkRenderer::New();
  ren->AddActor(coneActor);
  
  vtkRenderWindow *renWindow = vtkRenderWindow::New();
  renWindow->AddRenderer(ren);
  renWindow->SetSize( 300, 300 );

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWindow);  
  
  // draw the resulting scene
  renWindow->Render();
  
  //  Begin mouse interaction
  iren->Start();
  
  // Clean up
  ren->Delete();
  renWindow->Delete();
  iren->Delete();
  downStreamPort->Delete();
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
  controller->SetMultipleMethod(1, process_a, NULL);
  controller->SetMultipleMethod(0, process_b, NULL);
  controller->MultipleMethodExecute();

  controller->UnRegister(NULL);
}


