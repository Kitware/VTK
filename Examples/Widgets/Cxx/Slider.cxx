#include <vtkSphereSource.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkSliderWidget.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkCommand.h>
#include <vtkWidgetEvent.h>
#include <vtkCallbackCommand.h>
#include <vtkWidgetEventTranslator.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSliderWidget.h>
#include <vtkSliderRepresentation3D.h>
#include <vtkProperty.h>
 
// The callback does the work.
// The callback keeps a pointer to the sphere whose resolution is
// controlled. After constructing the callback, the program sets the
// SphereSource of the callback to 
// the object to be controlled.
class vtkSliderCallback : public vtkCommand
{
public:
  static vtkSliderCallback *New() 
    {
    return new vtkSliderCallback;
    }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
    vtkSliderWidget *sliderWidget = 
      reinterpret_cast<vtkSliderWidget*>(caller);
    int value = static_cast<int>(static_cast<vtkSliderRepresentation *>(sliderWidget->GetRepresentation())->GetValue());
    this->SphereSource->SetPhiResolution(value/2);
    this->SphereSource->SetThetaResolution(value);
    }
  vtkSliderCallback():SphereSource(0) {}
  vtkSphereSource *SphereSource;
};
 
int main (int, char *[])
{
  // A sphere
  vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->SetCenter(0.0, 0.0, 0.0);
  sphereSource->SetRadius(4.0);
  sphereSource->SetPhiResolution(4);
  sphereSource->SetThetaResolution(8);
 
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInput(sphereSource->GetOutput());
 
  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToFlat();
 
  // A renderer and render window
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
 
  // An interactor
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);
 
  // Add the actors to the scene
  renderer->AddActor(actor);
 
  // Render an image (lights and cameras are created automatically)
  renderWindow->Render();
 
  vtkSmartPointer<vtkSliderRepresentation3D> sliderRep = vtkSmartPointer<vtkSliderRepresentation3D>::New();
  sliderRep->SetMinimumValue(3.0);
  sliderRep->SetMaximumValue(50.0);
  sliderRep->SetValue(sphereSource->GetThetaResolution());
  sliderRep->SetTitleText("Sphere Resolution");
  sliderRep->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  sliderRep->GetPoint1Coordinate()->SetValue(-4,6,0);
  sliderRep->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  sliderRep->GetPoint2Coordinate()->SetValue(4,6,0);
  sliderRep->SetSliderLength(0.075);
  sliderRep->SetSliderWidth(0.05);
  sliderRep->SetEndCapLength(0.05);
 
  vtkSmartPointer<vtkSliderWidget> sliderWidget = vtkSmartPointer<vtkSliderWidget>::New();
  sliderWidget->SetInteractor(renderWindowInteractor);
  sliderWidget->SetRepresentation(sliderRep);
  sliderWidget->SetAnimationModeToAnimate();
  sliderWidget->EnabledOn();
 
  vtkSmartPointer<vtkSliderCallback> callback = vtkSmartPointer<vtkSliderCallback>::New();
  callback->SphereSource = sphereSource;
 
  sliderWidget->AddObserver(vtkCommand::InteractionEvent,callback);
 
  renderWindowInteractor->Initialize();
  renderWindow->Render();
 
  renderWindowInteractor->Start();
 
  return EXIT_SUCCESS;
}
