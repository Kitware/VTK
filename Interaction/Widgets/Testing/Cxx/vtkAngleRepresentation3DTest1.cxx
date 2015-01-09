#include "vtkAngleRepresentation3D.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkAngleRepresentation3DTest1(int , char * [] )
{
  vtkSmartPointer< vtkAngleRepresentation3D > node1 = vtkSmartPointer< vtkAngleRepresentation3D >::New();

  EXERCISE_BASIC_ANGLE_REPRESENTATION_METHODS(vtkAngleRepresentation3D, node1);

  vtkActor *actor = node1->GetRay1();
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
  vtkFollower *follower = node1->GetTextActor();
  if (follower == NULL)
    {
    std::cout << "Follower is null." << std::endl;
    }
  double scale[3] = {1.0, 2.0, 3.0};
  node1->SetTextActorScale(scale);
  double *retScale = node1->GetTextActorScale();
  if (retScale == NULL)
    {
    std::cerr << "Error in setting text actor scale, used " << scale[0] << ", " << scale[1] << ", " << scale[2] << " but got back NULL" << std::endl;
    return EXIT_FAILURE;
    }
  else if (retScale[0] != scale[0] ||
           retScale[1] != scale[1] ||
           retScale[2] != scale[2])
    {
    std::cerr << "Error in setting text actor scale, used " << scale[0] << ", " << scale[1] << ", " << scale[2] << " but got back " <<  retScale[0] << ", " << retScale[1] << ", " << retScale[2] << std::endl;
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
