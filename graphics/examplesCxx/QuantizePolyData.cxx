#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkQuantizePolyDataPoints.h"
#include "vtkPolyDataNormals.h"
#include "vtkActor.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  char a;
  
  // create a rendering window and renderer
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWindow = vtkRenderWindow::New();
    renWindow->AddRenderer(ren);
  renWindow->SetSize( 300, 300 );

  // create an actor and give it sphere geometry
  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(36);
    sphere->SetPhiResolution(18);
    sphere->SetRadius(1.0);

  vtkQuantizePolyDataPoints *quantizer = vtkQuantizePolyDataPoints::New();
    quantizer->SetQFactor(0.1);
    quantizer->SetInput(sphere->GetOutput());

  vtkPolyDataNormals *normalmaker = vtkPolyDataNormals::New();
    normalmaker->SetInput(quantizer->GetOutput());
    normalmaker->SetFeatureAngle(5.0);
    normalmaker->SetSplitting(1);

  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
    sphereMapper->SetInput(normalmaker->GetOutput());
  vtkActor *sphereActor = vtkActor::New();
    sphereActor->SetMapper(sphereMapper);

  // assign our actor to the renderer
  ren->AddActor(sphereActor);

  // draw the resulting scene
  renWindow->Render();

  SAVEIMAGE( renWindow );

  // loop until key is pressed
  cout << "Press any key followed by <Enter> to exit>> ";
  cin >> a;

  if(ren->GetActors()->IsItemPresent(sphereActor))
   {
   ren->RemoveActor(sphereActor);
   }
  sphereActor->Delete();
  sphereMapper->Delete();
  sphere->Delete();
  quantizer->Delete();
  normalmaker->Delete();

  renWindow->Delete();
  ren->Delete();

  // Clean up
}
