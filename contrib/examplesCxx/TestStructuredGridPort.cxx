// This program test the unstructured ports by setting up a simple pipeline.

#include "vtkPLOT3DReader.h"
#include "vtkGridSynchronizedTemplates3D.h"
#include "vtkOutputPort.h"
#include "vtkInputPort.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderWindowInteractor.h"

// arbitrary tags for communication
#define GRID_TAG         123

void process_a(vtkMultiProcessController *controller,void *vtkNotUsed(arg) )
{
  vtkPLOT3DReader *pl3d = vtkPLOT3DReader::New();
  pl3d->SetXYZFileName("../../../vtkdata/combxyz.bin");
  pl3d->SetQFileName("../../../vtkdata/combq.bin");
  pl3d->SetScalarFunctionNumber(100);
  pl3d->SetVectorFunctionNumber(202);
  
  vtkOutputPort *upStreamPort = vtkOutputPort::New();
  upStreamPort->SetInput(pl3d->GetOutput());
  upStreamPort->SetTag(GRID_TAG);
  
  // wait for the call back to execute.
  upStreamPort->WaitForUpdate();
  
  pl3d->Delete();
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
  downStreamPort->SetTag(GRID_TAG);

  vtkGridSynchronizedTemplates3D *iso = vtkGridSynchronizedTemplates3D::New();
  iso->SetInput(downStreamPort->GetStructuredGridOutput());
  iso->SetValue(0, 0.4);
  iso->SetNumberOfThreads(1);
  // Ask for half
  iso->GetOutput()->SetUpdateExtent(0, 2);
  iso->Update();

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput(iso->GetOutput());

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  
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
  
  // Clean up
  ren->Delete();
  renWindow->Delete();
  //iren->Delete();
  actor->Delete();
  mapper->Delete();
  iso->Delete();
  downStreamPort->Delete();

}


void main( int argc, char *argv[] )
{
  vtkMultiProcessController *controller;
  
  controller = vtkMultiProcessController::New();

  controller->Initialize(argc, argv);
  controller->SetNumberOfProcesses(2);
  controller->SetMultipleMethod(0, process_b, NULL);
  controller->SetMultipleMethod(1, process_a, NULL);
  controller->MultipleMethodExecute();

  controller->Delete();
}


