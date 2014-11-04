#include "vtkBalloonRepresentation.h"

#include <cstdlib>
#include <iostream>

#include "vtkImageData.h"
#include "vtkTextProperty.h"
#include "vtkProperty2D.h"

#include "WidgetTestingMacros.h"

int vtkBalloonRepresentationTest1(int , char * [] )
{
  vtkSmartPointer< vtkBalloonRepresentation > node1 = vtkSmartPointer< vtkBalloonRepresentation >::New();

  EXERCISE_BASIC_REPRESENTATION_METHODS(vtkBalloonRepresentation, node1);

  vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
  node1->SetBalloonImage(imageData);
  vtkSmartPointer<vtkImageData> imageData2 = node1->GetBalloonImage();
  if (imageData2 != imageData)
    {
    std::cerr << "Error in Set/Get ImageData" << std::endl;
    return EXIT_FAILURE;
    }
  TEST_SET_GET_STRING(node1, BalloonText);

  TEST_SET_GET_VECTOR2_INT_RANGE(node1, ImageSize, 0, 100);

  vtkSmartPointer<vtkTextProperty> textProp = vtkSmartPointer<vtkTextProperty>::New();
  node1->SetTextProperty(textProp);
  if (node1->GetTextProperty() != textProp)
    {
    std::cerr << "Failure in Set/Get TextProperty" << std::endl;
    return EXIT_FAILURE;
    }

  vtkSmartPointer<vtkProperty2D> frameProp = vtkSmartPointer<vtkProperty2D>::New();
  node1->SetFrameProperty(frameProp);
  if (node1->GetFrameProperty() != frameProp)
    {
    std::cerr << "Failure in Set/Get FrameProperty" << std::endl;
    return EXIT_FAILURE;
    }

  vtkSmartPointer<vtkProperty2D> imageProp = vtkSmartPointer<vtkProperty2D>::New();
  node1->SetImageProperty(imageProp);
  if (node1->GetImageProperty() != imageProp)
    {
    std::cerr << "Failure in Set/Get ImageProperty" << std::endl;
    return EXIT_FAILURE;
    }

  TEST_SET_GET_INT_RANGE(node1, BalloonLayout, 0, 3);
  node1->SetBalloonLayoutToImageLeft();
  node1->SetBalloonLayoutToImageRight();
  node1->SetBalloonLayoutToImageBottom();
  node1->SetBalloonLayoutToImageTop();
  node1->SetBalloonLayoutToTextLeft();
  node1->SetBalloonLayoutToTextRight();
  node1->SetBalloonLayoutToTextTop();
  node1->SetBalloonLayoutToTextBottom();

  TEST_SET_GET_VECTOR2_INT_RANGE(node1, Offset, -1, 1);
  TEST_SET_GET_INT_RANGE(node1, Padding, 1, 99);

  return EXIT_SUCCESS;
}
