// This program test the ports by setting up a simple pipeline.

#include "vtkOutputPort.h"
#include "vtkInputPort.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkElevationFilter.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkWindowToImageFilter.h"
#include "vtkTIFFWriter.h"

void process_a( vtkMultiProcessController *controller, void *vtkNotUsed(arg) )
{
  vtkConeSource *cone = vtkConeSource::New();
  vtkElevationFilter *elev = vtkElevationFilter::New();
  vtkOutputPort *upStreamPort = vtkOutputPort::New();
  
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
}


void  process_b( vtkMultiProcessController *controller, void *arg )
{
  int myid, otherid;
  char *save_filename = (char*)arg;

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
  downStreamPort->GetPolyDataOutput()->SetUpdateExtent(0, 4);
  downStreamPort->Update();  

  vtkPolyData *data =   downStreamPort->GetPolyDataOutput();
  
  vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
  coneMapper->SetInput(downStreamPort->GetPolyDataOutput());

  vtkActor *coneActor = vtkActor::New();
  coneActor->SetMapper(coneMapper);
  
  vtkRenderer *ren = vtkRenderer::New();
  ren->AddActor(coneActor);
  ren->SetBackground(0.1, 0.3, 0.5);
  
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);
  renWin->SetSize( 300, 300 );

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);  
  
  // draw the resulting scene
  renWin->Render();

  // save for the regression test
  if (save_filename != NULL && save_filename[0] != '\0')
    {
    vtkWindowToImageFilter *w2if = vtkWindowToImageFilter::New();
    vtkTIFFWriter *rttiffw = vtkTIFFWriter::New();
    w2if->SetInput(renWin);
    rttiffw->SetInput(w2if->GetOutput());
    rttiffw->SetFileName(save_filename); 
    rttiffw->Write(); 
    // Tell the other process to stop waiting.
    controller->TriggerRMI(otherid, vtkMultiProcessController::BREAK_RMI_TAG);
    exit(1);
    }
  else
    {
    //  Begin mouse interaction
    iren->Start();
    controller->TriggerRMI(otherid, vtkMultiProcessController::BREAK_RMI_TAG);
    }
  
  // Clean up
  ren->Delete();
  renWin->Delete();
  iren->Delete();
  downStreamPort->Delete();
  coneMapper->Delete();
  coneActor->Delete();

}


int main( int argc, char *argv[] )
{
  vtkMultiProcessController *controller;
  char save_filename[100];

  save_filename[0] = '\0';
  if( (argc >= 2) && (strcmp("-S", argv[argc-1]) == 0) )
    {
    sprintf( save_filename, "%s.cxx.tif", argv[0] );
    }
  
  controller = vtkMultiProcessController::New();

  controller->Initialize(&argc, &argv);
  controller->SetNumberOfProcesses(2);
  controller->SetMultipleMethod(0, process_b, save_filename);
  controller->SetMultipleMethod(1, process_a, NULL);
  controller->MultipleMethodExecute();

  controller->Finalize();
  controller->Delete();
  
  return 0;
}





