// This program test the ports by setting up a simple pipeline.

#include "mpi.h"
#include "vtkImageReader.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkAppendPolyData.h"
#include "vtkUpStreamPort.h"
#include "vtkDownStreamPort.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkElevationFilter.h"
#include "vtkRenderWindowInteractor.h"


VTK_THREAD_RETURN_TYPE process( void *vtkNotUsed(arg) )
{
  vtkMultiProcessController *controller;
  vtkImageReader *reader;
  vtkSynchronizedTemplates3D *iso;
  vtkElevationFilter *elev;
  int myid, numProcs;
  float val;
  
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);
  myid = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();
    
  reader = vtkImageReader::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 127, 0, 127, 1, 93);
  reader->SetFilePrefix("../../../vtkdata/headsq/half");
  reader->SetDataSpacing(1.6, 1.6, 1.5);
  
  iso = vtkSynchronizedTemplates3D::New();
  iso->SetInput(reader->GetOutput());
  iso->SetValue(0, 500);
  iso->ComputeScalarsOff();
  iso->ComputeGradientsOff();
  // This should be automatically determined by controller.
  iso->SetNumberOfThreads(1);
  
  // Compute a different color for each process.
  if (numProcs == 1) 
    {
    val = 0.0;
    } 
  else 
    {
    val = (float)(myid) / (float)(numProcs-1);
    }
  elev = vtkElevationFilter::New();
  elev->SetInput(iso->GetOutput());
  elev->SetScalarRange(val, val+0.001);

  if (myid != 0)
    {
    // Remote process! Send data throug port.
    vtkUpStreamPort *upPort = vtkUpStreamPort::New();
    

    upPort->SetInput(elev->GetPolyDataOutput());
    // the different process ids differentiate between sources.
    upPort->SetTag(999);
    // wait for the call back to execute.
    upPort->WaitForUpdate();
    
    // last call never returns, but ...
    upPort->Delete();
    }
  else
    {
    int i;
    vtkAppendPolyData *app = vtkAppendPolyData::New();
    vtkDownStreamPort *downPort;
    vtkRenderer *ren = vtkRenderer::New();
    vtkRenderWindow *renWindow = vtkRenderWindow::New();
    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
    vtkActor *actor = vtkActor::New();
    
    // This is the main thread: Collect the data and render it.
    app->AddInput(elev->GetPolyDataOutput());
    // ###################### important ####################
    // # this tells the append filter to request pieces from
    // # each of its inputs.
    app->ParallelStreamingOn();
    
    for (i = 1; i < numProcs; ++i)
      {
      downPort = vtkDownStreamPort::New();
      downPort->SetUpStreamProcessId(i);
      downPort->SetTag(999);
      app->AddInput(downPort->GetPolyDataOutput());
      // referenced by app ...
      downPort->Delete();
      downPort = NULL;
      }
    
    putenv("DISPLAY=:0.0");
    
    renWindow->AddRenderer(ren);
    iren->SetRenderWindow(renWindow);
    ren->SetBackground(0.9, 0.9, 0.9);
    renWindow->SetSize( 400, 400);
  
    mapper->SetInput(app->GetOutput());
    actor->SetMapper(mapper);
  
    // assign our actor to the renderer
    ren->AddActor(actor);
  
    ren->GetActiveCamera()->SetFocalPoint(100, 100, 65);
    ren->GetActiveCamera()->SetPosition(100, 450, 65);
    ren->GetActiveCamera()->SetViewUp(0, 0, -1);
    ren->GetActiveCamera()->SetViewAngle(30);
    ren->GetActiveCamera()->ComputeViewPlaneNormal();
    ren->ResetCameraClippingRange();

    // draw the resulting scene
    renWindow->Render();
    
    //  Begin mouse interaction
    iren->Start();
    
    // Clean up
    app->Delete();
    ren->Delete();
    renWindow->Delete();
    iren->Delete();
    mapper->Delete();
    actor->Delete();
    }
  
  // clean up objects in all processes.
  controller->UnRegister(NULL);
  reader->Delete();
  iso->Delete();
  elev->Delete();

  return VTK_THREAD_RETURN_VALUE;
}


void main( int argc, char *argv[] )
{
  vtkMultiProcessController *controller;
  int myid;
  
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);

  controller->Initialize(argc, argv);
  controller->SetSingleMethod(process, NULL);
  controller->SingleMethodExecute();

  controller->UnRegister(NULL);
}


