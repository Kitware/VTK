// This program test the ports by setting up a simple pipeline.

#include "mpi.h"
#include "vtkUpStreamPort.h"
#include "vtkDownStreamPort.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkElevationFilter.h"
#include "vtkRenderWindowInteractor.h"


void main( int argc, char *argv[] )
{
  char a;
  int numprocs, myid;
  int id0 = 0;
  int id1 = 1;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  cerr << "process: " << myid << " of " << numprocs << endl;
  
  // setup the pipeline in process 1
  if (myid == id1) {
    vtkConeSource *cone = vtkConeSource::New();
    vtkElevationFilter *elev = vtkElevationFilter::New();
    vtkUpStreamPort *upStreamPort = vtkUpStreamPort::New();
    
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


  // set up the renderer in process 0
  if (myid == id0) 
    {
    vtkRenderer *ren = vtkRenderer::New();
    vtkRenderWindow *renWindow = vtkRenderWindow::New();
    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    vtkDownStreamPort *downStreamPort = vtkDownStreamPort::New();
    vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
    
    renWindow->AddRenderer(ren);
    iren->SetRenderWindow(renWindow);
    renWindow->SetSize( 300, 300 );
    

    downStreamPort->SetUpStreamProcessId(id1);
    downStreamPort->SetTag(999);
    coneMapper->SetInput(downStreamPort->GetPolyDataOutput());
    vtkActor *coneActor = vtkActor::New();
    coneActor->SetMapper(coneMapper);
    
    // assign our actor to the renderer
    ren->AddActor(coneActor);
    
    // draw the resulting scene
    renWindow->Render();
    
    //  Begin mouse interaction
    iren->Start();
    
    // Clean up
    ren->Delete();
    renWindow->Delete();
    iren->Delete();
    downStreamPort->Delete();
    coneMapper->Delete();
    coneActor->Delete();
  }

  cerr << myid << " waiting at barrier\n";
  MPI_Barrier (MPI_COMM_WORLD);
  cerr << myid << " past barrier\n";
  MPI_Finalize();
  return(0);
}


