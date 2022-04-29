#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkLineRepresentation.h>
#include <vtkLineWidget2.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

#include <vtkInteractorEventRecorder.h>

static const char* eventLog = "# StreamVersion 1.1\n"
                              "ExposeEvent 0 189 0 0 0 0\n"
                              "LeftButtonPressEvent 379 253 0 0 0 0\n"
                              "MouseMoveEvent 380 253 0 0 0 0\n"
                              "MouseMoveEvent 749 5 0 0 0 0\n"
                              "LeftButtonReleaseEvent 749 5 0 0 0 0\n"
                              "MouseMoveEvent 746 12 0 0 0 0\n"
                              "MouseMoveEvent 371 249 0 0 0 0\n"
                              "LeftButtonPressEvent 371 249 0 0 0 0\n"
                              "MouseMoveEvent 370 250 0 0 0 0\n"
                              "MouseMoveEvent 23 479 0 0 0 0\n"
                              "LeftButtonReleaseEvent 23 479 0 0 0 0\n"
                              "MouseMoveEvent 23 478 0 0 0 0\n"
                              "MouseMoveEvent 572 110 0 0 0 0\n"
                              "LeftButtonPressEvent 572 110 0 0 0 0\n"
                              "MouseMoveEvent 572 111 0 0 0 0\n"
                              "MouseMoveEvent 578 139 0 0 0 0\n";

int TestLineWidget2b(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // We want to test the handle behavior of widget representation
  // when the camera is far away from the origin
  vtkNew<vtkSphereSource> sphereSource;

  vtkNew<vtkTransform> transform;
  transform->Translate(10000, 0, 0);
  transform->Scale(100000, 100000, 100000);

  vtkNew<vtkTransformFilter> transformFilter;
  transformFilter->SetInputConnection(sphereSource->GetOutputPort());
  transformFilter->SetTransform(transform);
  transformFilter->Update();

  double point1[3] = { 9500, 0, 0 };
  double point2[3] = { 10500, 0, 0 };
  vtkNew<vtkLineRepresentation> lineRepresentation;
  lineRepresentation->SetPoint1WorldPosition(point1);
  lineRepresentation->SetPoint2WorldPosition(point2);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(transformFilter->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  renderWindow->SetWindowName("TestLineWidget2b");
  renderWindow->SetSize(750, 500);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindowInteractor->Initialize();

  vtkNew<vtkLineWidget2> lineWidget;
  lineWidget->SetInteractor(renderWindowInteractor);
  lineWidget->SetRepresentation(lineRepresentation);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(renderWindowInteractor);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);

  renderWindow->Render();
  lineWidget->On();

  recorder->Play();
  recorder->Off();

  vtkNew<vtkPolyData> polydata;
  auto lineRepr = static_cast<vtkLineRepresentation*>(lineWidget->GetRepresentation());
  lineRepr->GetPolyData(polydata);

  double p1[3];
  polydata->GetPoint(0, p1);
  if (point1[0] == p1[0] && point1[0] == p1[0] && point1[0] == p1[0])
  {
    std::cerr << "Wrong coordinate value for Point1. Expected {-113226, 81112.1, 0} but got {"
              << p1[0] << ", " << p1[1] << ", " << p1[2] << "}" << std::endl;
    return EXIT_FAILURE;
  }

  double p2[3];
  polydata->GetPoint(polydata->GetNumberOfPoints() - 1, p2);
  if (point2[0] != p2[0] || point2[0] != p2[0] || point2[0] != p2[0])
  {
    std::cerr << "Wrong coordinate value for Point2. Expected {10500, 0, 0} but got {" << p2[0]
              << ", " << p2[1] << ", " << p2[2] << "}" << std::endl;
    return EXIT_FAILURE;
  }

  double distance = lineRepr->GetDistance();
  if (abs(distance - 147943.6465) > 0.0001)
  {
    std::cerr << "Wrong distance. Expected 147943 but got " << distance << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
