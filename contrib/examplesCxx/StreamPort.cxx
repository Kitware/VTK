// This program test streaming through ports.

#include "vtkImageReader.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkPolyDataCollector.h"
#include "vtkOutputPort.h"
#include "vtkInputPort.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkMath.h"



// callback to test streaming / ports by seeing what extents are being read in.
void reader_start_callback(void *arg)
{
  vtkImageReader *reader = (vtkImageReader*)(arg);
  int *e;
  
  e = reader->GetOutput()->GetUpdateExtent();
  
  cerr << "Reading: " << e[0] << ", " << e[1] << ", " << e[2] << ", " 
       << e[3] << ", " << e[4] << ", " << e[5] << endl; 
}


// callback to see if iso has data
void iso_end_callback(void *arg)
{
  vtkPolyData *out = (vtkPolyData*)(arg);
  
  cerr << "iso out: " << *out << endl;
}


void process_a(vtkMultiProcessController *controller, void *vtkNotUsed(arg) )
{
  vtkImageReader *reader;
  vtkSynchronizedTemplates3D *iso;
  
    
  reader = vtkImageReader::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 127, 0, 127, 1, 93);
  reader->SetFilePrefix("../../../vtkdata/headsq/half");
  reader->SetDataSpacing(1.6, 1.6, 1.5);
  reader->SetStartMethod(reader_start_callback, (void*)(reader));
  
  iso = vtkSynchronizedTemplates3D::New();
  iso->SetInput(reader->GetOutput());
  iso->SetValue(0, 500);
  iso->ComputeScalarsOff();
  iso->ComputeGradientsOff();
  //iso->SetEndMethod(iso_end_callback, (void*)(iso->GetOutput()));
  // This should be automatically determined by controller.
  iso->SetNumberOfThreads(1);

  // Send data throug port.
  vtkOutputPort *upPort = vtkOutputPort::New();
  upPort->SetInput(iso->GetOutput());
  upPort->SetTag(999);
    
  // wait for the call back to execute.
  upPort->WaitForUpdate();
    
  // last call never returns, but ...
  upPort->Delete();
}


void process_b(vtkMultiProcessController *controller, void *vtkNotUsed(arg) )
{
  vtkInputPort *downPort;
  vtkPolyDataCollector *collector = vtkPolyDataCollector::New();
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  vtkActor *actor = vtkActor::New();
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWindow = vtkRenderWindow::New();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  vtkCamera *cam = vtkCamera::New();
  int myid, otherid;
  
  myid = controller->GetLocalProcessId();
  otherid = ( ! myid);
  
  downPort = vtkInputPort::New();
  downPort->SetRemoteProcessId(otherid);
  downPort->SetTag(999);
  
  collector->SetInput(downPort->GetPolyDataOutput());
  collector->SetInputMemoryLimit(1000);
  
  mapper->SetInput(collector->GetOutput());
  actor->SetMapper(mapper);
  
  putenv("DISPLAY=:0.0");
  
  renWindow->AddRenderer(ren);
  iren->SetRenderWindow(renWindow);
  ren->SetBackground(0.9, 0.9, 0.9);
  renWindow->SetSize( 400, 400);
  
  // assign our actor to the renderer
  ren->AddActor(actor);
  
  cam->SetFocalPoint(100, 100, 65);
  cam->SetPosition(100, 450, 65);
  cam->SetViewUp(0, 0, -1);
  cam->SetViewAngle(30);
  // this was causing an update.
  //ren->ResetCameraClippingRange();
  //{
  //double *range = ren->GetActiveCamera()->GetClippingRange();
  //cerr << range[0] << ", " << range[1] << endl;
  //}
  cam->SetClippingRange(177.0, 536.0);
  ren->SetActiveCamera(cam);
  
  collector->Update();
  collector->Update();
  renWindow->Render();
  
  // just exit
  //controller->TriggerRMI(otherid, VTK_BREAK_RMI_TAG);      
  //exit(0);
  
  //  Begin mouse interaction
  iren->Start();
  
  // Clean up
  collector->Delete();
  ren->Delete();
  renWindow->Delete();
  iren->Delete();
  mapper->Delete();
  actor->Delete();
  
}


void main( int argc, char *argv[] )
{
  vtkMultiProcessController *controller;
  
  controller = vtkMultiProcessController::New();

  controller->Initialize(argc, argv);
  controller->SetNumberOfProcesses(2);
  controller->SetMultipleMethod(1, process_a, NULL);
  controller->SetMultipleMethod(0, process_b, NULL);
  controller->MultipleMethodExecute();

  controller->Delete();
}

  




