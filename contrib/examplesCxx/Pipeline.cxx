// This program tests pipeline parallism.
// I can only run with two processors (because of communication lockup).
// To extend to three processes,  connect
// process C's port to process B's port.
// You will also have to prime process B's port somehow.

#include "mpi.h"
#include "vtkImageGaussianSource.h"
#include "vtkImageEllipsoidSource.h"
#include "vtkUpStreamPort.h"
#include "vtkDownStreamPort.h"
#include "vtkImageShiftScale.h"
#include "vtkTexture.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#define ID_A 1
#define ID_B 2
#define ID_C 0

// Used to change the source (to get a series of images for the pipeline)
void callback1(void *arg, int id)
{
  vtkImageGaussianSource *source = (vtkImageGaussianSource *)arg;
  float max = source->GetMaximum();
  source->SetMaximum(max + 10.0);
}

// End Execute methods of filters so we can examine values of
// the outputs.  Arg is the output image data.
void report(void *arg)
{
  vtkMultiProcessController *controller;
  int myid;
  char process;
  int center = 256 * 128 + 128;
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);
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
  
  controller->UnRegister(NULL);
}


VTK_THREAD_RETURN_TYPE process_a( void *vtkNotUsed(arg) )
{
  // Set up the pipeline source.
  vtkImageGaussianSource *source = vtkImageGaussianSource::New();
  source->SetCenter(128.0, 128.0, 0.0);
  source->SetMaximum(10.0);
  source->SetStandardDeviation(50.0);
  source->SetEndMethod(report, source->GetOutput());

  vtkUpStreamPort *upStreamPort = vtkUpStreamPort::New();
  upStreamPort->SetInput(source->GetOutput());
  upStreamPort->SetTag(888);
  upStreamPort->PipelineFlagOn();
  
  // Put in a call back that allows process C to change the series.
  vtkMultiProcessController *controller;
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);
  controller->AddRMI(callback1, source, 300);
  controller->UnRegister(NULL);
  
  // wait for the call back to execute.
  upStreamPort->WaitForUpdate();
  
  source->Delete();
  upStreamPort->Delete();

  return VTK_THREAD_RETURN_VALUE;
}


VTK_THREAD_RETURN_TYPE process_b( void *vtkNotUsed(arg) )
{
  vtkDownStreamPort *downStreamPort = vtkDownStreamPort::New();
  downStreamPort->SetUpStreamProcessId(ID_A);
  downStreamPort->SetTag(888);
  
  vtkImageShiftScale *scale = vtkImageShiftScale::New();
  scale->SetInput(downStreamPort->GetImageDataOutput());
  scale->SetScale(0.1);
  scale->SetEndMethod(report, scale->GetOutput());
  
  vtkUpStreamPort *upStreamPort = vtkUpStreamPort::New();  
  upStreamPort->SetInput(scale->GetOutput());
  upStreamPort->SetTag(999);
  
  // wait for the call back to execute.
  upStreamPort->WaitForUpdate();
  
  downStreamPort->Delete();
  scale->Delete();
  upStreamPort->Delete();

  return VTK_THREAD_RETURN_VALUE;
}


VTK_THREAD_RETURN_TYPE process_c( void *vtkNotUsed(arg) )
{
  vtkMultiProcessController *controller;
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);
  int idx;
  
  putenv("DISPLAY=:0.0");

  vtkDownStreamPort *downStreamPort = vtkDownStreamPort::New();
  downStreamPort->SetUpStreamProcessId(ID_A);
  downStreamPort->SetTag(888);

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

  // draw the resulting scene
  cerr << "----------------------\n";
  downStreamPort->Update();
  sleep(5);
  controller->TriggerRMI(ID_A, 300);
  cerr << "----------------------\n";
  scale->Update();
  sleep(5);
  controller->TriggerRMI(ID_A, 300);
  cerr << "----------------------\n";
  scale->Update();
  sleep(5);
  controller->TriggerRMI(ID_A, 300);
  cerr << "----------------------\n";
  scale->Update();
  sleep(5);
  controller->TriggerRMI(ID_A, 300);  
  cerr << "----------------------\n";
  scale->Update();
  sleep(5);
  cerr << "----------------------\n";
  scale->Update();
  sleep(5);
  
  controller->TriggerRMI(ID_A, VTK_BREAK_RMI_TAG);
  controller->TriggerRMI(ID_B, VTK_BREAK_RMI_TAG);
  
  // Clean up
  //ren->Delete();
  //renWindow->Delete();
  atext->Delete();
  plane->Delete();
  mapper->Delete();
  actor->Delete();
  downStreamPort->Delete();
  controller->UnRegister(NULL);

  return VTK_THREAD_RETURN_VALUE;
}


void main( int argc, char *argv[] )
{
  vtkMultiProcessController *controller;
  
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);
  
  controller->Initialize(argc, argv);
  controller->SetNumberOfProcesses(3);
  controller->SetMultipleMethod(ID_A, process_a, NULL);
  controller->SetMultipleMethod(ID_B, process_b, NULL);
  controller->SetMultipleMethod(ID_C, process_c, NULL);
  controller->MultipleMethodExecute();

  controller->UnRegister(NULL);
}


