#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCubeSource.h"
#include "vtkSphereSource.h"

#include "SaveImage.h"

int main( int argc, char *argv[] )
{
  // create rendering windows and three renderers
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderer *ren2 = vtkRenderer::New();
  vtkRenderWindow *renWindow1 = vtkRenderWindow::New();
    renWindow1->AddRenderer(ren1);
    renWindow1->AddRenderer(ren2);
  vtkRenderWindowInteractor *iren1 = vtkRenderWindowInteractor::New();
    iren1->SetRenderWindow(renWindow1);

  vtkRenderer *ren3 = vtkRenderer::New();
  vtkRenderWindow *renWindow2 = vtkRenderWindow::New();
    renWindow2->AddRenderer(ren3);
  vtkRenderWindowInteractor *iren2 = vtkRenderWindowInteractor::New();
    iren2->SetRenderWindow(renWindow2);

  // create an actor and give it cone geometry
  vtkConeSource *cone = vtkConeSource::New();
     cone->SetResolution(8);
  vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
    coneMapper->SetInput(cone->GetOutput());
  vtkActor *coneActor = vtkActor::New();
    coneActor->SetMapper(coneMapper);
    coneActor->GetProperty()->SetColor(0.2000,0.6300,0.7900);

  // create an actor and give it cube geometry
  vtkCubeSource *cube = vtkCubeSource::New();
  vtkPolyDataMapper *cubeMapper = vtkPolyDataMapper::New();
    cubeMapper->SetInput(cube->GetOutput());
  vtkActor *cubeActor = vtkActor::New();
    cubeActor->SetMapper(cubeMapper);
    cubeActor->GetProperty()->SetColor(0.9804,0.5020,0.4471);

  // create an actor and give it sphere geometry
  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(16); sphere->SetPhiResolution(16);
  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
    sphereMapper->SetInput(sphere->GetOutput());
  vtkActor *sphereActor = vtkActor::New();
    sphereActor->SetMapper(sphereMapper);
    sphereActor->GetProperty()->SetColor(0.8900,0.6600,0.4100);

  // assign our actor to both renderers
  ren1->AddActor(coneActor);
  ren2->AddActor(sphereActor);
  ren3->AddActor(cubeActor);

  // set the size of our window
  renWindow1->SetSize(300,150);
  renWindow1->SetPosition(0,50);
  renWindow2->SetSize(300,300);
  renWindow2->SetPosition(0,300);

  // set the viewports and background of the renderers
  ren1->SetViewport(0,0,0.5,1);
  ren1->SetBackground(0.9,0.9,0.9);
  ren2->SetViewport(0.5,0,1,1);
  ren2->SetBackground(1,1,1);
  ren3->SetBackground(1,1,1);

  // draw the resulting scene
  renWindow1->Render();
  renWindow2->Render();

  SAVEIMAGE( renWindow1 );

  iren1->Start();

  // Clean up
  ren1->Delete();
  ren2->Delete();
  renWindow1->Delete();
  iren1->Delete();
  ren3->Delete();
  renWindow2->Delete();
  iren2->Delete();
  cone->Delete();
  coneMapper->Delete();
  coneActor->Delete();
  cube->Delete();
  cubeMapper->Delete();
  cubeActor->Delete();
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();
}
