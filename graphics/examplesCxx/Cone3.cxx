#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  // create a rendering window and renderer
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWindow = vtkRenderWindow::New();
    renWindow->AddRenderer(ren);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWindow);
  renWindow->SetSize( 300, 300 );

  // create an actor and give it cone geometry
  vtkConeSource *cone = vtkConeSource::New();
    cone->SetResolution(8);
  vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
    coneMapper->SetInput(cone->GetOutput());
  vtkActor *coneActor = vtkActor::New();
    coneActor->SetMapper(coneMapper);

  // assign our actor to the renderer
  ren->AddActor(coneActor);

  // draw the resulting scene
  renWindow->Render();

  SAVEIMAGE( renWindow );

  //  Begin mouse interaction
  iren->Start();

  // Clean up
  ren->Delete();
  renWindow->Delete();
  iren->Delete();
  cone->Delete();
  coneMapper->Delete();
  coneActor->Delete();
}
