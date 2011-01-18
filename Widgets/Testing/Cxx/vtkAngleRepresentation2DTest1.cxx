#include "vtkAngleRepresentation2D.h"

#include <stdlib.h>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkAngleRepresentation2DTest1(int , char * [] )
{
  vtkSmartPointer< vtkAngleRepresentation2D > node1 = vtkSmartPointer< vtkAngleRepresentation2D >::New();

  EXERCISE_BASIC_ANGLE_REPRESENTATION_METHODS(vtkAngleRepresentation2D, node1);

  vtkLeaderActor2D *actor = node1->GetRay1();
  if (actor == NULL)
    {
    std::cout << "Ray 1 is null." << std::endl;
    }
  actor = node1->GetRay2();
  if (actor == NULL)
    {
    std::cout << "Ray 2 is null." << std::endl;
    }
  actor = node1->GetArc();
  if (actor == NULL)
    {
    std::cout << "Arc is null." << std::endl;
    }
  return EXIT_SUCCESS;
}
