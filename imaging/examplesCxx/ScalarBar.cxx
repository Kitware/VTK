#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkLookupTable.h"
#include "vtkScalarBarActor.h"

int main ()
{
 vtkRenderWindow *renWin;
 vtkRenderer *ren1;
 vtkRenderWindowInteractor *iren;
 vtkSphereSource *sphere;
 vtkPolyDataMapper *map;
 vtkActor *aSphere;

 // create a window, renderer and interactor
 renWin = vtkRenderWindow::New();
 ren1 = vtkRenderer::New();
   renWin->AddRenderer(ren1);
 iren = vtkRenderWindowInteractor::New();
   iren->SetRenderWindow(renWin);

 // create sphere geometry
 sphere = vtkSphereSource::New();
   sphere->SetRadius(1.0);
   sphere->SetThetaResolution(18);
   sphere->SetPhiResolution(18);

 // Add a vtkLookupTable
 vtkLookupTable* wat = vtkLookupTable::New();
   wat->SetTableRange(-5,5);
   wat->Build();
 // map to graphics library
 map = vtkPolyDataMapper::New();
   map->SetInput(sphere->GetOutput());
   map->SetScalarRange(sphere->GetOutput()->GetScalarRange());
 // actor coordinates geometry, properties, transformation
 aSphere = vtkActor::New();
   aSphere->SetMapper(map);
   aSphere->GetProperty()->SetColor(0,0,1);// sphere color blue

 ren1->AddActor(aSphere);
 ren1->SetBackground(1,1,1); // Background color white

 // Create a scalar bar
 vtkScalarBarActor *scalarBar = vtkScalarBarActor::New();
   scalarBar->SetLookupTable(wat);
   scalarBar->SetTitle("Temperature");
   scalarBar->SetPosition(0, 0);
   scalarBar->SetOrientationToHorizontal();

 ren1->AddActor2D(scalarBar);
 // Render an image; since no lights/cameras specified, created automatically
 renWin->Render();

  renWin->SetFileName("blah.ppm");
 // renWin->SaveImageAsPPM();

 //  Begin mouse interaction
 iren->Start();
 return 0;
}
