// This program test the ports by setting up a simple pipeline.

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

#define ID_A 0
#define ID_B 1
#define ID_C 2

// Used to change the source (to get a series of images for the pipeline)
void callback1(void *arg, int id)
{
  vtkImageGaussianSource *source = (vtkImageGaussianSource *)arg;
  float max = source->GetMaximum();
  source->SetMaximum(max + 100.0);
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
  source->SetMaximum(100.0);
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
  
  // Prime the pipeline
  upStreamPort->GetInput()->Update();
  controller->TriggerRMI(ID_A, 300);
  cerr << "----------------------\n";
  
  // wait for the call back to execute.
  upStreamPort->WaitForUpdate();
  
  source->Delete();
  upStreamPort->Delete();

  return VTK_THREAD_RETURN_VALUE;
}


VTK_THREAD_RETURN_TYPE process_b( void *vtkNotUsed(arg) )
{
  vtkMultiProcessController *controller;
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);
  
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
  upStreamPort->PipelineFlagOn();
  
  // Prime the pipeline
  upStreamPort->GetInput()->Update();
  controller->TriggerRMI(ID_A, 300);  
  cerr << "----------------------\n";
  
  upStreamPort->WaitForUpdate();
  
  downStreamPort->Delete();
  scale->Delete();
  upStreamPort->Delete();
  controller->UnRegister(NULL);

  return VTK_THREAD_RETURN_VALUE;
}


VTK_THREAD_RETURN_TYPE process_c( void *vtkNotUsed(arg) )
{
  vtkMultiProcessController *controller;
  controller = vtkMultiProcessController::RegisterAndGetGlobalController(NULL);
  int idx;
  
  putenv("DISPLAY=:0.0");

  vtkDownStreamPort *downStreamPort = vtkDownStreamPort::New();
  downStreamPort->SetUpStreamProcessId(ID_B);
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

  // Pipeline is all primed, now start processing
  scale->Update();
  controller->TriggerRMI(ID_A, 300);
  sleep(1);
  cerr << "----------------------\n";
  scale->Update();
  controller->TriggerRMI(ID_A, 300);
  sleep(1);
  cerr << "----------------------\n";
  scale->Update();
  controller->TriggerRMI(ID_A, 300);  
  sleep(1);
  cerr << "----------------------\n";
  
  // Now empty the data buffered in the pipeline
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


