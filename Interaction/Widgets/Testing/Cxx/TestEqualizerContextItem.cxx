#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextTransform.h"
#include "vtkContextView.h"
#include "vtkEqualizerContextItem.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestEqualizerContextItem(int, char*[])
{
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(800, 600);

  vtkNew<vtkContextTransform> transform;
  transform->SetInteractive(true);
  transform->Identity();
  view->GetScene()->AddItem(transform);

  vtkNew<vtkEqualizerContextItem> equalizerItem;
  equalizerItem->SetTransform(transform);
  view->GetScene()->AddItem(equalizerItem);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  auto points = equalizerItem->GetPoints();
  if (points != "0,1;500,1;")
  {
    std::cout << "Get default points..FAILED" << std::endl;
    return EXIT_FAILURE;
  }

  std::string newPoints = "0,0;100,100;50,25.75;";
  equalizerItem->SetPoints(newPoints);
  std::string returnedPoints = equalizerItem->GetPoints();
  if (returnedPoints != newPoints)
  {
    std::cout << "Converting points..FAILED" << std::endl;
    std::cout << "Original points = " << newPoints << std::endl;
    std::cout << "Returned points = " << returnedPoints << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
