#include "vtkTextRepresentation.h"

#include <stdlib.h>
#include <iostream>

#include "vtkImageData.h"
#include "vtkTextProperty.h"
#include "vtkProperty2D.h"

#include "WidgetTestingMacros.h"

#include <vtkTextActor.h>
#include <vtkPointHandleRepresentation3D.h>

int vtkTextRepresentationTest1(int , char * [] )
{
  vtkSmartPointer< vtkTextRepresentation > node1 = vtkSmartPointer< vtkTextRepresentation >::New();

  EXERCISE_BASIC_BORDER_REPRESENTATION_METHODS(vtkTextRepresentation, node1);



  vtkSmartPointer<vtkTextActor> textActor = vtkSmartPointer<vtkTextActor>::New();
  node1->SetTextActor(textActor);
  if (node1->GetTextActor() != textActor)
    {
    std::cerr << "Failure in Get/Set TextActor." << std::endl;
    return EXIT_FAILURE;
    }

  TEST_SET_GET_INT_RANGE(node1, WindowLocation, 0, 6);

  double pos[2] = {-99.0, 100.0};
  node1->SetPosition(pos);
  double *pos2;
  pos2 = node1->GetPosition();
  if (!pos2 ||
      pos2[0] != pos[0] ||
      pos2[1] != pos[1])
    {
    std::cerr << "Failure in Get/Set Position, expected " << pos[0] << ", " << pos[1] << ", instead got " << pos2[0] << ", " << pos2[1] << std::endl;
    return EXIT_FAILURE;
    }
  pos[1] = 99.0;
  node1->SetPosition(pos[0], pos[1]);
  pos2 = node1->GetPosition();
  if (!pos2 ||
      pos2[0] != pos[0] ||
      pos2[1] != pos[1])
    {
    std::cerr << "Failure in Get/Set Position x,y, expected " << pos[0] << ", " << pos[1] << ", instead got " << pos2[0] << ", " << pos2[1] << std::endl;
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
