// This program demonstrates the use ports by setting up a simple 
// pipeline. All processes create an identical pipeline:
// vtkImageReader -> vtkContourFilter -> vtkElevationFilter
// In addition, the first (root) process creates n input ports
// (where n=nProcs-1), each attached to an output port on the other 
// processes. It then appends the polygonal output from all input 
// ports and it's own pipeline and renders the result ISO_NUM times,
// each time setting a different scalar value to be contoured.

#include "vtkImageReader.h"
#include "vtkContourFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkOutputPort.h"
#include "vtkInputPort.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkElevationFilter.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTimerLog.h"
#include "vtkMath.h"
#include "vtkWindowToImageFilter.h"
#include "vtkTIFFWriter.h"
#include "vtkMultiProcessController.h"

static const float ISO_START=4250.0;
static const float ISO_STEP=-1250.0;
static const int ISO_NUM=3;
// Just pick a tag which is available
static const int ISO_VALUE_RMI_TAG=300; 
static const int PORT_TAG=999;

// call back to set the iso surface value.
void SetIsoValueRMI(void *localArg, void *remoteArg, 
		  int remoteArgLen, int id)
{ 
  float val;

  vtkContourFilter *iso;
  iso = (vtkContourFilter *)localArg;
  val = iso->GetValue(0);
  iso->SetValue(0, val + ISO_STEP);
}


// This will be called by all processes
void MyMain( vtkMultiProcessController *controller, void *arg )
{
  vtkImageReader *reader;
  vtkContourFilter *iso;
  vtkElevationFilter *elev;
  int myid, numProcs;
  float val;
  int numTris;
  char *save_filename = (char*)arg;
  
  // Obtain the id of the running process and the total
  // number of processes
  myid = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();
    
  // Create the reader, the data file name might have
  // to be changed depending on where the data files are.
  reader = vtkImageReader::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 127, 0, 127, 1, 93);
  reader->SetFilePrefix("../../../../vtkdata/headsq/half");
  reader->SetDataSpacing(1.6, 1.6, 1.5);

  // Iso-surface.
  iso = vtkContourFilter::New();
  iso->SetInput(reader->GetOutput());
  iso->SetValue(0, ISO_START);
  iso->ComputeScalarsOff();
  iso->ComputeGradientsOff();
  
  // Compute a different color for each process.
  elev = vtkElevationFilter::New();
  elev->SetInput(iso->GetOutput());
  vtkMath::RandomSeed(myid * 100);
  val = vtkMath::Random();
  elev->SetScalarRange(val, val+0.001);

  if (myid != 0)
    {
    // If I am not the root process

    // Satellite process! Send data through port.
    vtkOutputPort *upPort = vtkOutputPort::New();
    
    // Last, set up a RMI call back to change the iso surface value.
    // This is done so that the root process can let this process
    // know that it wants the contour value to change.
    controller->AddRMI(SetIsoValueRMI, (void *)iso, ISO_VALUE_RMI_TAG);
  
    // connect the port to the output of the pipeline
    upPort->SetInput(elev->GetPolyDataOutput());

    // Multiple ports can go through the same connection.
    // This is used to differentiate ports
    upPort->SetTag(PORT_TAG);

    // Loop which processes RMI requests. 
    // Use vtkMultiProcessController::BREAK_RMI_TAG to break it.
    // The root process with send a ISO_VALUE_RMI_TAG to make this
    // process change it's contour value.
    upPort->WaitForUpdate();
    
    // We are done. Clean up.
    upPort->Delete();
    }
  else
    {
    // If I am the root process

    int i, j;
    vtkAppendPolyData *app = vtkAppendPolyData::New();
    vtkInputPort *downPort;
    vtkRenderer *ren = vtkRenderer::New();
    vtkRenderWindow *renWindow = vtkRenderWindow::New();
    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
    vtkActor *actor = vtkActor::New();
    vtkTimerLog *timer = vtkTimerLog::New();
    vtkCamera *cam = vtkCamera::New();

    // Add my pipeline's output to the append filter
    app->AddInput(elev->GetPolyDataOutput());

    // ###################### important ####################
    // # This tells the append filter to request pieces from
    // # each of its inputs.  Since each of its inputs comes from
    // # a different process,  each process generates a separate 
    // # piece of the data (data parallelism).
    // # If this is not used, all processes will iso-surface
    // # all the data.
    app->ParallelStreamingOn();
    
    // This is the main thread: Collect the data and render it.
    for (i = 1; i < numProcs; ++i)
      {
      downPort = vtkInputPort::New();
      downPort->SetRemoteProcessId(i);

      // Multiple ports can go through the same connection.
      // This is used to differentiate ports
      downPort->SetTag(PORT_TAG);

      app->AddInput(downPort->GetPolyDataOutput());

      // Reference already incremented by AddInput(). Delete()
      // will only decrement the count, not destroy the object.
      // The ports will be destroyed when the append filter
      // goes away.
      downPort->Delete();
      downPort = NULL;
      }

    // Create the rendering part of the pipeline
    renWindow->AddRenderer(ren);
    iren->SetRenderWindow(renWindow);
    ren->SetBackground(0.9, 0.9, 0.9);
    renWindow->SetSize( 400, 400);
  
    mapper->SetInput(app->GetOutput());
    actor->SetMapper(mapper);
  
    ren->AddActor(actor);
  
    cam->SetFocalPoint(100, 100, 65);
    cam->SetPosition(100, 450, 65);
    cam->SetViewUp(0, 0, -1);
    cam->SetViewAngle(30);

    cam->SetClippingRange(177.0, 536.0);
    ren->SetActiveCamera(cam);
    
    // loop through some iso surface values.
    for (j = 0; j < ISO_NUM; ++j)
      {
      // set the local value
      SetIsoValueRMI((void*)iso, NULL, 0, 0);
      for (i = 1; i < numProcs; ++i)
	{
	// trigger the RMI to change the iso surface value.
	controller->TriggerRMI(i, ISO_VALUE_RMI_TAG);      
	}
      
      // Time the rendering. Note that the execution on all processes
      // start only after Update()
      timer->StartTimer();
      app->Update();
      timer->StopTimer();
      numTris = iso->GetOutput()->GetNumberOfCells();
      val = iso->GetValue(0);
      cerr << "Update " << val << " took " << timer->GetElapsedTime() 
	   << " seconds to produce " << numTris << " triangles\n";
      
      // now render the results
      renWindow->Render();
      }

    if (save_filename[0] != '\0')
      {
      // Save an image if we need to for regression test.
      vtkWindowToImageFilter *w2if = vtkWindowToImageFilter::New();
      vtkTIFFWriter *rttiffw = vtkTIFFWriter::New();
      w2if->SetInput(renWindow);
      rttiffw->SetInput(w2if->GetOutput());
      rttiffw->SetFileName(save_filename); 
      rttiffw->Write(); 
      }
    else
      {
      //  Begin mouse interaction
      iren->Start();
      }

    // Tell the other processors to stop processing RMIs.
    for (i = 1; i < numProcs; ++i)
      {
      controller->TriggerRMI(i, vtkMultiProcessController::BREAK_RMI_TAG); 
      }
    
    // Clean up
    app->Delete();
    ren->Delete();
    renWindow->Delete();
    iren->Delete();
    mapper->Delete();
    actor->Delete();
    }
  
  // clean up objects in all processes.
  reader->Delete();
  iso->Delete();
  elev->Delete();
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

  controller->SetSingleMethod(MyMain, save_filename);

  if (controller->IsA("vtkThreadedController"))
    {
    controller->SetNumberOfProcesses(2);
    } 
  controller->SingleMethodExecute();
  
  controller->Finalize();
  controller->Delete();
  return 0;
}





