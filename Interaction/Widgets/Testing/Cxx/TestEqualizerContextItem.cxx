#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextTransform.h"
#include "vtkContextView.h"
#include "vtkEqualizerContextItem.h"
#include "vtkFloatArray.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkNew.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

constexpr const char* testingEvents1 = "# StreamVersion 1.1\n"
                                       // Move camera
                                       "LeftButtonPressEvent 268 264 0 0 0 0\n"
                                       "MouseMoveEvent 268 264 0 0 0 0\n"
                                       "MouseMoveEvent 386 426 0 0 0 0\n"
                                       "LeftButtonReleaseEvent 386 426 0 0 0 0\n"
                                       // Create a point on the widget and move it
                                       "LeftButtonPressEvent 281 161 0 0 0 0\n"
                                       "MouseMoveEvent 281 162 0 0 0 0\n"
                                       "MouseMoveEvent 275 220 0 0 0 0\n"
                                       "LeftButtonReleaseEvent 275 220 0 0 0 0\n";
constexpr const char* expectedPoints1 = "0,1;157,58;500,1;";

constexpr const char* testingEvents2 =
  // Erase the newly selected point
  "RightButtonPressEvent 275 220 0 0 0 0\n"
  "RightButtonReleaseEvent 275 220 0 0 0 0\n";
constexpr const char* expectedPoints2 = "0,1;500,1;";

int TestEqualizerContextItem(int, char*[])
{
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(800, 600);
  view->GetRenderWindow()->SetMultiSamples(0);

  vtkNew<vtkContextTransform> transform;
  transform->SetInteractive(true);
  transform->Identity();
  view->GetScene()->AddItem(transform);

  vtkNew<vtkEqualizerContextItem> equalizerItem;
  equalizerItem->SetTransform(transform);
  view->GetScene()->AddItem(equalizerItem);

  vtkNew<vtkInteractorEventRecorder> eventPlayer;
  eventPlayer->SetInteractor(view->GetInteractor());
  eventPlayer->ReadFromInputStringOn();

  // Create a new point on the widget using a single click and move it
  eventPlayer->SetInputString(testingEvents1);
  eventPlayer->Play();
  auto points = equalizerItem->GetPoints();
  if (points != expectedPoints1)
  {
    std::cout << "ERROR: after creating a point, expected '" << expectedPoints1 << "' but got '"
              << points << "'" << std::endl;
    return EXIT_FAILURE;
  }

  // Remove the point we created using right click
  eventPlayer->SetInputString(testingEvents2);
  eventPlayer->Play();
  points = equalizerItem->GetPoints();
  if (points != expectedPoints2)
  {
    std::cout << "ERROR: after removing a point, expected '" << expectedPoints1 << "' but got '"
              << points << "'" << std::endl;
    return EXIT_FAILURE;
  }

  // Test raw API
  std::string newPoints = "0,0;100,100;50,25.75;";
  equalizerItem->SetPoints(newPoints);
  std::string returnedPoints = equalizerItem->GetPoints();
  if (returnedPoints != newPoints)
  {
    std::cout << "ERROR: Converting points failed" << std::endl;
    std::cout << " - Original points: " << newPoints << std::endl;
    std::cout << " - Returned points: " << returnedPoints << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
