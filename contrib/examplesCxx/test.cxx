// This program test the ports by setting up a simple pipeline.

#include "mpi.h"
#include "vtkImageReader.h"
#include "vtkImageSobel3D.h"
#include "vtkImageGaussianSmooth.h"
#include "vtkImageNormalize.h"
#include "vtkImageShrink3D.h"
#include "vtkImageMagnitude.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkAppendPolyData.h"
#include "vtkMultiProcessController.h"
#include "vtkUpStreamPort.h"
#include "vtkDownStreamPort.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTimerLog.h"
#include "vtkMath.h"




// global timer log used to compute all times
vtkTimerLog *TIMER = NULL;

// global times used to collect times.
float TIME_ARRAY[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

// start and end methods to compute times.
void start_method(void *arg)
{
  TIMER->StartTimer();
}
void end_method(void *arg)
{
  int *p = (int *)arg;
  
  TIMER->StopTimer();
  TIME_ARRAY[*p] = TIMER->GetElapsedTime();
}

// call back to get times
void get_times_rmi(void *arg, int id)
{ 
  vtkMultiProcessController *controller;
  
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);
  controller->Send(TIME_ARRAY, 7, id, 1234567);
  
  controller->UnRegister(NULL);
}

// call back to set the iso surface value.
void set_iso_val_rmi(void *arg, int id)
{ 
  float val;
  vtkSynchronizedTemplates3D *iso = (vtkSynchronizedTemplates3D*)(arg);
  
  // receive the iso surface value from the main thread.
  val = iso->GetValue(0);
  iso->SetValue(0, val+0.05);
  
}

// call back to set the correlation neighborhood.
void set_smooth_std_rmi(void *arg, int id)
{ 
  float val, *std;
  vtkImageGaussianSmooth *smooth = (vtkImageGaussianSmooth*)(arg);
  
  std = smooth->GetStandardDeviations();
  val = std[0] + 1.0;
  smooth->SetStandardDeviations(val, val, val);
}

// call back to exit program
// This should really be embedded in the controller.
void exit_rmi(void *arg, int id)
{ 
  // clean up controller ?
  exit(0);
}



VTK_THREAD_RETURN_TYPE process( void *vtkNotUsed(arg) )
{
  vtkMultiProcessController *controller;
  vtkImageReader *reader;
  vtkImageSobel3D *sobel;
  vtkImageNormalize *norm;
  vtkImageGaussianSmooth *smooth;
  vtkImageShrink3D *shrink;
  vtkImageMagnitude *mag;
  vtkSynchronizedTemplates3D *iso;
  vtkTimerLog *timer = vtkTimerLog::New();
  int myid, numProcs;
  int idxs[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  float tmp[10];
  
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);
  myid = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();
    
  reader = vtkImageReader::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 127, 0, 127, 1, 93);
  reader->SetFilePrefix("../../../vtkdata/headsq/half");
  reader->SetDataSpacing(1.6, 1.6, 1.5);
  reader->SetStartMethod(start_method, NULL);
  reader->SetEndMethod(end_method, idxs+0); 
  reader->GetOutput()->ReleaseDataFlagOn();

  // create a vector field
  sobel = vtkImageSobel3D::New();
  sobel->SetInput(reader->GetOutput());
  sobel->SetNumberOfThreads(1);
  sobel->SetStartMethod(start_method, NULL);
  sobel->SetEndMethod(end_method, idxs+1); 
  sobel->GetOutput()->ReleaseDataFlagOn();
  
  norm = vtkImageNormalize::New();
  norm->SetInput(sobel->GetOutput());
  norm->SetNumberOfThreads(1);
  norm->SetStartMethod(start_method, NULL);
  norm->SetEndMethod(end_method, idxs+2); 
  norm->GetOutput()->ReleaseDataFlagOn();
  
  smooth = vtkImageGaussianSmooth::New();
  smooth->SetInput(norm->GetOutput());
  smooth->SetDimensionality(3);
  smooth->SetStandardDeviations(2.0, 2.0, 2.0);
  smooth->SetRadiusFactors(1, 1, 1);
  smooth->SetNumberOfThreads(1);
  smooth->SetStartMethod(start_method, NULL);
  smooth->SetEndMethod(end_method, idxs+3); 
  smooth->GetOutput()->ReleaseDataFlagOn();
  
  shrink = vtkImageShrink3D::New();
  shrink->SetInput(smooth->GetOutput());
  shrink->SetShrinkFactors(2, 2, 2);
  shrink->SetNumberOfThreads(1);
  shrink->SetStartMethod(start_method, NULL);
  shrink->SetEndMethod(end_method, idxs+4); 
  shrink->GetOutput()->ReleaseDataFlagOn();
  
  mag = vtkImageMagnitude::New();
  mag->SetInput(shrink->GetOutput());
  mag->SetNumberOfThreads(1);
  mag->SetStartMethod(start_method, NULL);
  mag->SetEndMethod(end_method, idxs+5); 
  mag->GetOutput()->ReleaseDataFlagOff();
  
  iso = vtkSynchronizedTemplates3D::New();
  iso->SetInput(mag->GetOutput());
  iso->SetValue(0, 0.6);
  iso->ComputeScalarsOn();
  iso->ComputeNormalsOn();
  // This should be automatically determined by controller.
  iso->SetNumberOfThreads(1);
  iso->SetStartMethod(start_method, NULL);
  iso->SetEndMethod(end_method, idxs+6);
  iso->GetOutput()->ReleaseDataFlagOn();
  

  //====================================================================
  if (myid != 0)
    {
    // Remote process! Send data throug port.
    vtkUpStreamPort *upPort = vtkUpStreamPort::New();
    
    // set up a RMI call back to change the neighborhood
    controller->AddRMI(set_smooth_std_rmi, (void *)smooth, 299);
    // set up a RMI call back to change the iso surface value.
    controller->AddRMI(set_iso_val_rmi, (void *)iso, 300);
    // rmi to get times from other process
    controller->AddRMI(get_times_rmi, NULL, 301);
    // rmi to exit
    controller->AddRMI(exit_rmi, (void *)iso, 302);
  
    upPort->SetInput(iso->GetOutput());
    upPort->SetStartMethod(start_method, NULL);
    upPort->SetEndMethod(end_method, idxs+7);

  // the different process ids differentiate between sources.
    upPort->SetTag(999);
    // wait for the call back to execute.
    upPort->WaitForUpdate();
    
    // last call never returns, but ...
    upPort->Delete();
    }
    //====================================================================
  else
    {
    //====================================================================
    int i, j, k;
    vtkAppendPolyData *app = vtkAppendPolyData::New();
    vtkDownStreamPort *downPort;
    vtkRenderer *ren = vtkRenderer::New();
    vtkRenderWindow *renWindow = vtkRenderWindow::New();
    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
    vtkActor *actor = vtkActor::New();
    vtkCamera *cam = vtkCamera::New();
    char filename[500];
    
    // This is the main thread: Collect the data and render it.
    app->AddInput(iso->GetOutput());
    // ###################### important ####################
    // # this tells the append filter to request pieces from
    // # each of its inputs.
    app->ParallelStreamingOn();
    
    for (i = 1; i < numProcs; ++i)
      {
      downPort = vtkDownStreamPort::New();
      downPort->SetUpStreamProcessId(i);
      downPort->SetTag(999);
      downPort->SetStartMethod(start_method, NULL);
      downPort->SetEndMethod(end_method, idxs+8);
      downPort->GetPolyDataOutput()->ReleaseDataFlagOn();
      
      app->AddInput(downPort->GetPolyDataOutput());
      // referenced by app ...
      downPort->Delete();
      downPort = NULL;
      }
    app->SetStartMethod(start_method, NULL);
    app->SetEndMethod(end_method, idxs+9);
    
    putenv("DISPLAY=:0.0");
    
    renWindow->AddRenderer(ren);
    iren->SetRenderWindow(renWindow);
    ren->SetBackground(0.9, 0.9, 0.9);
    renWindow->SetSize( 400, 400);
  
    mapper->SetInput(app->GetOutput());
    mapper->ImmediateModeRenderingOn();
    actor->SetMapper(mapper);
  
    // assign our actor to the renderer
    ren->AddActor(actor);
  
    cam->SetFocalPoint(100, 100, 65);
    cam->SetPosition(100, 650, 65);
    cam->SetViewUp(0, 0, -1);
    cam->SetViewAngle(30);
    cam->ComputeViewPlaneNormal();
    // this was causing an update.
    //ren->ResetCameraClippingRange();
    //{
    //double *range = ren->GetActiveCamera()->GetClippingRange();
    //cerr << range[0] << ", " << range[1] << endl;
    //}
    cam->SetClippingRange(177.0, 536.0);
    ren->SetActiveCamera(cam);
    
    // loop through some iso surface values.
    for (j = 0; j < 5; ++j)
      {
      // set the local value
      //set_iso_val_rmi(iso, 0);
      set_smooth_std_rmi(smooth, 0);
      for (i = 1; i < numProcs; ++i)
	{
	// trigger the RMI to change the iso surface value.
	//controller->TriggerRMI(i, 300);      
	controller->TriggerRMI(i, 299);      
	}
      
      timer->StartTimer();
      app->Update();
      timer->StopTimer();
      cerr << "Total Update Time: " << timer->GetElapsedTime() << " seconds\n";
      
      // compile itemized times
      for (i = 1; i < numProcs; ++i)
	{
	controller->TriggerRMI(i, 301);
	controller->Receive(tmp, 7, i, 1234567);
	
	// take maximums (only the first eight are across all processes).
	for (k = 0; k < 8; ++k)
	  {
	  if (TIME_ARRAY[k] < tmp[k])
	    {
	    TIME_ARRAY[k] = tmp[k];
	    }  
	  }
	}
      
      // transfer time is contained in both up and down port times?!?
      if (j == 0)
	{ // reader is only valid for the first update.
	cerr << "  reader max:     \t" << TIME_ARRAY[0] << " seconds\n";
	cerr << "  grad max:    \t" << TIME_ARRAY[1] << " seconds\n";
	cerr << "  norm max:    \t" << TIME_ARRAY[2] << " seconds\n";
	}
      // Take these out of conditional if you change deviation each iter. 
      cerr << "  smooth max:  \t" << TIME_ARRAY[3] << " seconds\n";
      cerr << "  shrink max:  \t" << TIME_ARRAY[4] << " seconds\n";
      cerr << "  mag max:     \t" << TIME_ARRAY[5] << " seconds\n";
      // ---
      cerr << "  iso max:     \t" << TIME_ARRAY[6] << " seconds\n";
      cerr << "  up port max: \t" << TIME_ARRAY[7] << " seconds\n";
      cerr << "  down port:   \t" << TIME_ARRAY[8] << " seconds\n";
      cerr << "  append:      \t" << TIME_ARRAY[9] << " seconds\n";
      
      
      // now render the results
      renWindow->Render();
      sprintf(filename, "flow_%d.ppm", j + 1000);
      renWindow->SetFileName(filename);
      renWindow->SaveImageAsPPM();
      }
    
    // just exit
    for (i = 1; i < numProcs; ++i)
      {
      // trigger the RMI to exit
      controller->TriggerRMI(i, 302);      
      }
    exit(0);
    
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
  //====================================================================
  
  // clean up objects in all processes.
  controller->UnRegister(NULL);
  reader->Delete();
  iso->Delete();

  return VTK_THREAD_RETURN_VALUE;
}


void main( int argc, char *argv[] )
{
  vtkMultiProcessController *controller;
  int myid;
  
  TIMER = vtkTimerLog::New();
  
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);

  controller->Initialize(argc, argv);
  controller->SetSingleMethod(process, NULL);
  controller->SingleMethodExecute();

  controller->UnRegister(NULL);
}





