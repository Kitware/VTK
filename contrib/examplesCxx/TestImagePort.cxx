// This program test the ports by setting up a simple pipeline.

#include "mpi.h"
#include "vtkUpStreamPort.h"
#include "vtkDownStreamPort.h"
#include "vtkImageGaussianSource.h"
#include "vtkImageEllipsoidSource.h"
#include "vtkImageViewer.h"
#include "vtkImageInformation.h"

VTK_THREAD_RETURN_TYPE process_a( void *vtkNotUsed(arg) )
{
  vtkMultiProcessController *controller;
  vtkImageGaussianSource *source = vtkImageGaussianSource::New();
  vtkImageEllipsoidSource *ellipse = vtkImageEllipsoidSource::New();
  vtkUpStreamPort *upStreamPort = vtkUpStreamPort::New();
  int myid;
  
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);
  myid = controller->GetLocalProcessId();
  
  // Set up the pipeline source.
  source->SetCenter(128.0, 128.0, 0.0);
  source->SetMaximum(2.0);
  source->SetStandardDeviation(50.0);

  ellipse->SetCenter(128.0, 128.0, 0.0);
  ellipse->SetRadius(50.0, 70.0, 1.0);

  upStreamPort->SetInput(source->GetOutput());
  upStreamPort->SetTag(999);
  
  // wait for the call back to execute.
  upStreamPort->WaitForUpdate();
  
  source->Delete();
  upStreamPort->Delete();

  return VTK_THREAD_RETURN_VALUE;
}


VTK_THREAD_RETURN_TYPE process_b( void *vtkNotUsed(arg) )
{
  vtkMultiProcessController *controller;
  vtkDownStreamPort *downStreamPort = vtkDownStreamPort::New();
  vtkImageViewer *viewer = vtkImageViewer::New();
  int myid, otherid;

  //putenv("DISPLAY=:0.0");
  
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

  downStreamPort->SetUpStreamProcessId(otherid);
  downStreamPort->SetTag(999);
  viewer->SetInput(downStreamPort->GetImageDataOutput());
  viewer->SetColorWindow(1.0);
  viewer->SetColorLevel(0.5);

  downStreamPort->Update();

  cerr << *(downStreamPort->GetImageDataOutput()) << endl;

  viewer->Render();

  //sginap(1000);
  //sleep(10);
  
  downStreamPort->Delete();
  viewer->Delete();

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


