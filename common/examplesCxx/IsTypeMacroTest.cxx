#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkCubeSource.h"
#include "vtkActor.h"

#include "SaveImage.h"

void main( int argc, char *argv[] )
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

  vtkCubeSource *cube = vtkCubeSource::New();
    cube->SetXLength(1.0);
    cube->SetYLength(1.0);
    cube->SetZLength(1.0);

  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
  vtkActor *sphereActor = vtkActor::New();

  bool t1,t2,t3,t4,t5,t6,t7,t8,t9,ta,tb,tc;
  t1 =  vtkCubeSource::IsTypeOf("vtkSource");
  t2 =  vtkSphereSource::IsTypeOf("vtkProcessObject");
  t3 =  vtkPolyDataMapper::IsTypeOf("vtkMapper");
  t4 =  vtkActor::IsTypeOf("vtkObject");
  t5 =  sphereActor->IsA("vtkObject");
  t6 =  sphereActor->IsA("vtkProp");
  t7 = !sphereActor->IsA("vtkCell");
  t8 = !sphereActor->IsA("vtkMapper");
  t9 = !sphereMapper->IsA("vtkSource");
  ta = !vtkPolyDataMapper::IsTypeOf("vtkProp");
  tb = !vtkPolyDataMapper::IsTypeOf("vtkRenderer");
  tc = !vtkRenderWindow::IsTypeOf("vtkRenderer");

  if (t1 && t2 && t3 && t4 && t5 && t6 && t7 &&
    t8 && t9 && ta && tb && tc)
  {
    sphereMapper->SetInput(sphere->GetOutput());
  }
  else
  {
    sphereMapper->SetInput(cube->GetOutput());
  }

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
  cube->Delete();

  renWindow->Delete();
  ren->Delete();

  // Clean up
}
