// This program test pipeline parallelism.  Output port changes its behavior
// with the PipelineFlagOn() call.  The output port has a call back that change
// the parameters of the source generating the series.

#include "mpi.h"
#include "vtkImageGaussianSource.h"
#include "vtkImageEllipsoidSource.h"
#include "vtkOutputPort.h"
#include "vtkInputPort.h"
#include "vtkImageShiftScale.h"
#include "vtkTexture.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#define ID_A 0
#define ID_B 1
#define ID_C 2

// RMI used to change the source (to get a series of images for the pipeline)
void change_param(void *arg)
{
  vtkImageGaussianSource *source = (vtkImageGaussianSource *)arg;
  float max = source->GetMaximum();
  if (max < 500.0)
    {
    source->SetMaximum(max + 100.0);
    }
}

// End Execute methods of filters so we can examine values of
// the outputs.  Arg is the output image data.
void report(void *arg)
{
  vtkMultiProcessController *controller;
  int myid;
  char process;
  int center = 256 * 128 + 128;
  controller = vtkMultiProcessController::GetGlobalController();
  myid = controller->GetLocalProcessId();
  if (myid == ID_A) {process = 'A';}
  if (myid == ID_B) {process = 'B';}
  if (myid == ID_C) {process = 'C';}
  
  vtkImageData *out = (vtkImageData *)(arg);
  
  vtkScalars *scalars = out->GetPointData()->GetScalars();
  if (scalars == NULL || scalars->GetNumberOfScalars() <= center)
    {
    cerr << process << " out = NULL\n";
    }
  else
    {
    cerr << process << " out = " << scalars->GetScalar(center) << endl;
    }
}


void process_a( vtkMultiProcessController *controller, void *vtkNotUsed(arg) )
{
  // Pipeline parallelism oprerates asynchronously so shallow copy does not work.
  controller->ForceDeepCopyOn();

  // Set up the pipeline source.
  vtkImageGaussianSource *source = vtkImageGaussianSource::New();
  source->SetCenter(128.0, 128.0, 0.0);
  source->SetMaximum(100.0);
  source->SetStandardDeviation(50.0);
  source->SetEndMethod(report, source->GetOutput());

  vtkOutputPort *upStreamPort = vtkOutputPort::New();
  upStreamPort->SetInput(source->GetOutput());
  upStreamPort->SetTag(888);
  // This flag changes the behavoir of the port.
  // It acts like a buffer that delays one iteraction.
  // It also calls update twice.  The first Update generates
  // the requested data and transfers it.  The seconds
  // starts processing the next request, but does not block the 
  // down stream port.
  upStreamPort->PipelineFlagOn();
  // This method is call to change the serries parameter.
  upStreamPort->SetParameterMethod(change_param, source);
  
  // wait for the call back to execute.
  upStreamPort->WaitForUpdate();
  
  source->Delete();
  upStreamPort->Delete();
}


void process_b(vtkMultiProcessController *controller, void *vtkNotUsed(arg) )
{
  // Pipeline parallelism oprerates asynchronously so shallow copy does not work.
  controller->ForceDeepCopyOn();
  
  vtkInputPort *downStreamPort = vtkInputPort::New();
  downStreamPort->SetRemoteProcessId(ID_A);
  downStreamPort->SetTag(888);
  
  vtkImageShiftScale *scale = vtkImageShiftScale::New();
  scale->SetInput(downStreamPort->GetImageDataOutput());
  scale->SetScale(0.1);
  scale->SetEndMethod(report, scale->GetOutput());
  
  vtkOutputPort *upStreamPort = vtkOutputPort::New();  
  upStreamPort->SetInput(scale->GetOutput());
  upStreamPort->SetTag(999);
  upStreamPort->PipelineFlagOn();
  
  upStreamPort->WaitForUpdate();
  
  downStreamPort->Delete();
  scale->Delete();
  upStreamPort->Delete();
}


void process_c(vtkMultiProcessController *controller, void *vtkNotUsed(arg) )
{
  int idx;

  // Pipeline parallelism oprerates asynchronously so shallow copy does not work.
  controller->ForceDeepCopyOn();

  putenv("DISPLAY=:0.0");

  vtkInputPort *downStreamPort = vtkInputPort::New();
  downStreamPort->SetRemoteProcessId(ID_B);
  downStreamPort->SetTag(999);

  vtkImageShiftScale *scale = vtkImageShiftScale::New();
  scale->SetInput(downStreamPort->GetImageDataOutput());
  scale->SetScale(0.1);
  scale->SetNumberOfThreads(1);
  scale->SetEndMethod(report, scale->GetOutput());

  vtkTexture *atext = vtkTexture::New();
  atext->SetInput(scale->GetOutput());
  atext->InterpolateOn();

  vtkPlaneSource *plane = vtkPlaneSource::New();
  vtkPolyDataMapper  *mapper = vtkPolyDataMapper::New();
  mapper->SetInput(plane->GetOutput());
  
  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  actor->SetTexture(atext);
  
  // assign our actor to the renderer
  vtkRenderer *ren = vtkRenderer::New();
  ren->AddActor(actor);
  
  vtkRenderWindow *renWindow = vtkRenderWindow::New();
  renWindow->AddRenderer(ren);
  renWindow->SetSize( 300, 300 );

  // Start processing
  scale->Update();
  sleep(1);
  cerr << "----------------------\n";
  scale->Update();
  sleep(1);
  cerr << "----------------------\n";
  scale->Update();
  sleep(1);
  cerr << "----------------------\n";
  scale->Update();
  sleep(1);
  cerr << "----------------------\n";
  scale->Update();
  sleep(1);
  cerr << "----------------------\n";
  
  controller->TriggerRMI(ID_A, VTK_BREAK_RMI_TAG);
  controller->TriggerRMI(ID_B, VTK_BREAK_RMI_TAG);
  
  // Clean up
  ren->Delete();
  renWindow->Delete();
  scale->Delete();
  atext->Delete();
  plane->Delete();
  mapper->Delete();
  actor->Delete();
  downStreamPort->Delete();
}


void main( int argc, char *argv[] )
{
  vtkMultiProcessController *controller;
  
  controller = vtkMultiProcessController::New();
  
  controller->Initialize(argc, argv);
  controller->SetNumberOfProcesses(3);
  controller->SetMultipleMethod(ID_A, process_a, NULL);
  controller->SetMultipleMethod(ID_B, process_b, NULL);
  controller->SetMultipleMethod(ID_C, process_c, NULL);
  controller->MultipleMethodExecute();

  controller->Delete();
}


