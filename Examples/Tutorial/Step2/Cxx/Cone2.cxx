//
// This example shows how to add an observer to a C++ program
//

// first include the required header files for the vtk classes we are using
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkCommand.h"

class myCallback : public vtkCommand
{
  virtual void Execute(vtkObject *caller, unsigned long, void *callData)
    {
    cerr << "Starting to Render\n";
    }
};

int main( int argc, char *argv[] )
{
  //
  // The pipeline creation is documented in Step1
  //
  vtkConeSource *cone = vtkConeSource::New();
  cone->SetHeight( 3.0 );
  cone->SetRadius( 1.0 );
  cone->SetResolution( 10 );

  vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
  coneMapper->SetInput( cone->GetOutput() );
  vtkActor *coneActor = vtkActor::New();
  coneActor->SetMapper( coneMapper );

  vtkRenderer *ren1= vtkRenderer::New();
  ren1->AddActor( coneActor );
  ren1->SetBackground( 0.1, 0.2, 0.4 );

  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer( ren1 );
  renWin->SetSize( 300, 300 );

  // Here is where we setup the observer, we do a new and ren1 will
  // eventually free the observer
  myCallback *mo1 = new myCallback;
  ren1->AddObserver(vtkCommand::StartEvent,mo1);
  
  //
  // now we loop over 360 degreeees and render the cone each time
  //
  int i;
  for (i = 0; i < 360; ++i)
    {
    // render the image
    renWin->Render();
    // rotate the active camera by one degree
    ren1->GetActiveCamera()->Azimuth( 1 );
    }
  
  //
  // Free up any objects we created
  //
  cone->Delete();
  coneMapper->Delete();
  coneActor->Delete();
  ren1->Delete();
  renWin->Delete();

  return 0;
}


