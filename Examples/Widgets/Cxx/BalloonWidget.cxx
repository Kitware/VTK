#include <vtkActor.h>
#include <vtkBalloonRepresentation.h>
#include <vtkBalloonWidget.h>
#include <vtkCommand.h> 
#include <vtkInteractorStyleTrackball.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkRegularPolygonSource.h>
 
int main (int, char *[])
{
  // Sphere
  vtkSmartPointer<vtkSphereSource> sphereSource =
    vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->SetCenter(-4.0, 0.0, 0.0);
  sphereSource->SetRadius(4.0);
 
  vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
 
  vtkSmartPointer<vtkActor> sphereActor =
    vtkSmartPointer<vtkActor>::New();
  sphereActor->SetMapper(sphereMapper);
 
  // Regular Polygon
  vtkSmartPointer<vtkRegularPolygonSource> regularPolygonSource =
    vtkSmartPointer<vtkRegularPolygonSource>::New();
  regularPolygonSource->SetCenter(4.0, 0.0, 0.0);
  regularPolygonSource->SetRadius(4.0);
 
  vtkSmartPointer<vtkPolyDataMapper> regularPolygonMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  regularPolygonMapper->SetInputConnection(regularPolygonSource->GetOutputPort());
 
  vtkSmartPointer<vtkActor> regularPolygonActor =
    vtkSmartPointer<vtkActor>::New();
  regularPolygonActor->SetMapper(regularPolygonMapper);
 
  // A renderer and render window
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
 
  // An interactor
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);
 
  // Create the widget
  vtkSmartPointer<vtkBalloonRepresentation> balloonRep =
    vtkSmartPointer<vtkBalloonRepresentation>::New();
  balloonRep->SetBalloonLayoutToImageRight();
 
  vtkSmartPointer<vtkBalloonWidget> balloonWidget =
    vtkSmartPointer<vtkBalloonWidget>::New();
  balloonWidget->SetInteractor(renderWindowInteractor);
  balloonWidget->SetRepresentation(balloonRep);
  balloonWidget->AddBalloon(sphereActor,
                            "This is a sphere",NULL);
  balloonWidget->AddBalloon(regularPolygonActor,
                            "This is a regular polygon",NULL);

  // Add the actors to the scene
  renderer->AddActor(sphereActor);
  renderer->AddActor(regularPolygonActor);

  // Render an image (lights and cameras are created automatically)
  renderWindow->Render();
  balloonWidget->EnabledOn();
 
  // Begin mouse interaction
  renderWindowInteractor->Start();
 
  return EXIT_SUCCESS;
}
