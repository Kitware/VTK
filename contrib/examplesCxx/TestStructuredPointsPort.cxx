// This program test the ports by setting up a simple pipeline.

#include "vtkImageGaussianSource.h"
#include "vtkImageEllipsoidSource.h"
#include "vtkImageToStructuredPoints.h"
#include "vtkOutputPort.h"
#include "vtkInputPort.h"
#include "vtkTexture.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"


void process_a(vtkMultiProcessController *controller, void *vtkNotUsed(arg) )
{
  vtkImageGaussianSource *source = vtkImageGaussianSource::New();
  vtkImageEllipsoidSource *ellipse = vtkImageEllipsoidSource::New();
  vtkOutputPort *upStreamPort = vtkOutputPort::New();
  
  // Set up the pipeline source.
  source->SetCenter(128.0, 128.0, 0.0);
  source->SetMaximum(2.0);
  source->SetStandardDeviation(50.0);

  ellipse->SetCenter(128.0, 128.0, 0.0);
  ellipse->SetRadius(50.0, 70.0, 1.0);
  
  vtkImageToStructuredPoints *sp = vtkImageToStructuredPoints::New();
  sp->SetInput(source->GetOutput());

  upStreamPort->SetInput((vtkImageData*)(sp->GetOutput()));
  upStreamPort->SetTag(999);
  
  // wait for the call back to execute.
  upStreamPort->WaitForUpdate();
  
  source->Delete();
  upStreamPort->Delete();
}


void process_b(vtkMultiProcessController *controller, void *vtkNotUsed(arg) )
{
  int myid, otherid;
  
  //putenv("DISPLAY=:0.0");
  
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

  vtkTexture *atext = vtkTexture::New();
  atext->SetInput(downStreamPort->GetStructuredPointsOutput());
  //atext->SetInput(downStreamPort->GetStructuredPointsOutput());
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

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWindow);  
  
  // draw the resulting scene
  renWindow->Render();
  
  //  Begin mouse interaction
  iren->Start();
  controller->TriggerRMI(otherid, VTK_BREAK_RMI_TAG);

  // Clean up
  ren->Delete();
  renWindow->Delete();
  iren->Delete();
  downStreamPort->Delete();
  atext->Delete();
  plane->Delete();
  mapper->Delete();
  actor->Delete();
}


int main( int argc, char *argv[] )
{
  vtkMultiProcessController *controller;
  
  controller = vtkMultiProcessController::New();

  controller->Initialize(&argc, &argv);
  controller->SetNumberOfProcesses(2);
  controller->SetMultipleMethod(0, process_b, NULL);
  controller->SetMultipleMethod(1, process_a, NULL);
  controller->MultipleMethodExecute();

  controller->Finalize();
  controller->Delete();

  return 0;
}


