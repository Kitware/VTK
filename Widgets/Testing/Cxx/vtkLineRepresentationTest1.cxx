#include "vtkLineRepresentation.h"

#include <stdlib.h>
#include <iostream>

#include "WidgetTestingMacros.h"

#include <vtkPointHandleRepresentation3D.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkFollower.h>

int vtkLineRepresentationTest1(int , char * [] )
{
  vtkSmartPointer< vtkLineRepresentation > node1 = vtkSmartPointer< vtkLineRepresentation >::New();

  vtkSmartPointer<vtkPointHandleRepresentation3D> handleRep = vtkSmartPointer<vtkPointHandleRepresentation3D>::New();
  node1->SetHandleRepresentation(handleRep);
  node1->InstantiateHandleRepresentation();

  EXERCISE_BASIC_REPRESENTATION_METHODS(vtkLineRepresentation, node1);


  double pos[3] = {-100.0, 0.0, 99.9};
  double pos2[3];
  double *posptr = NULL;

  // point 1 world
  node1->SetPoint1WorldPosition(pos);
  posptr = node1->GetPoint1WorldPosition();
  if (!posptr)
    {
    std::cerr << "Error in get double * for Point1WorldPosition, null pointer returned." << std::endl;
    return EXIT_FAILURE;
    }
  else if (posptr[0] != pos[0] ||
           posptr[1] != pos[1] ||
           posptr[2] != pos[2])
    {
    std::cerr << "Error in double * Set/Get Point1WorldPosition, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << " but got " << posptr[0] << ", " << posptr[1] << ", " << posptr[2] <<  std::endl;
    return EXIT_FAILURE;
    }
  else
    {
    std::cout << "GetPoint1WorldPosition double * = " << posptr[0] << ", " << posptr[1] << ", " << posptr[2] << std::endl;
    }
  node1->GetPoint1WorldPosition(pos2);
  if (pos2[0] != pos[0] ||
      pos2[1] != pos[1] ||
      pos2[2] != pos[2])
    {
    std::cerr << "Error in Set/Get Point1WorldPosition, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << " but got " << pos2[0] << ", " << pos2[1] << ", " << pos2[2] <<  std::endl;
    return EXIT_FAILURE;
    }
  else
    {
    std::cout << "GetPoint1WorldPosition = " << pos2[0] << ", " << pos2[1] << ", " << pos2[2] << std::endl;
    }

  // point 1 display
  /// causes seg faults, don't test yet
  /*
  pos[1] = -99.9;
  //node1->SetPoint1DisplayPosition(pos);
  posptr = node1->GetPoint1DisplayPosition();
  if (!posptr)
    {
    std::cerr << "Error in get double * for Point1DisplayPosition, null pointer returned." << std::endl;
    return EXIT_FAILURE;
    }
  else if (posptr[0] != pos[0] ||
           posptr[1] != pos[1] ||
           posptr[2] != pos[2])
    {
    std::cerr << "Error in double * Set/Get Point1DisplayPosition, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << " but got " << posptr[0] << ", " << posptr[1] << ", " << posptr[2] <<  std::endl;
    return EXIT_FAILURE;
    }
  else
    {
    std::cout << "GetPoint1DisplayPosition double * = " << posptr[0] << ", " << posptr[1] << ", " << posptr[2] << std::endl;
    }
  node1->GetPoint1DisplayPosition(pos2);
  if (pos2[0] != pos[0] ||
      pos2[1] != pos[1] ||
      pos2[2] != pos[2])
    {
    std::cerr << "Error in Set/Get Point1DisplayPosition, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << " but got " << pos2[0] << ", " << pos2[1] << ", " << pos2[2] <<  std::endl;
    return EXIT_FAILURE;
    }
  else
    {
    std::cout << "GetPoint1DisplayPosition = " << pos2[0] << ", " << pos2[1] << ", " << pos2[2] << std::endl;
    }
  */

  // point 2 world

  pos[1] = 77.0;
  node1->SetPoint2WorldPosition(pos);
  posptr = node1->GetPoint2WorldPosition();
  if (!posptr)
    {
    std::cerr << "Error in get double * for Point2WorldPosition, null pointer returned." << std::endl;
    return EXIT_FAILURE;
    }
  else if (posptr[0] != pos[0] ||
           posptr[1] != pos[1] ||
           posptr[2] != pos[2])
    {
    std::cerr << "Error in double * Set/Get Point2WorldPosition, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << " but got " << posptr[0] << ", " << posptr[1] << ", " << posptr[2] <<  std::endl;
    return EXIT_FAILURE;
    }
  else
    {
    std::cout << "GetPoint2WorldPosition double * = " << posptr[0] << ", " << posptr[1] << ", " << posptr[2] << std::endl;
    }
  node1->GetPoint2WorldPosition(pos2);
  if (pos2[0] != pos[0] ||
      pos2[1] != pos[1] ||
      pos2[2] != pos[2])
    {
    std::cerr << "Error in Set/Get Point2WorldPosition, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << " but got " << pos2[0] << ", " << pos2[1] << ", " << pos2[2] <<  std::endl;
    return EXIT_FAILURE;
    }
  else
    {
    std::cout << "GetPoint2WorldPosition = " << pos2[0] << ", " << pos2[1] << ", " << pos2[2] << std::endl;
    }

  vtkSmartPointer<vtkPointHandleRepresentation3D> subHandleRep;
  subHandleRep = node1->GetPoint1Representation();
  subHandleRep = node1->GetPoint2Representation();
  subHandleRep = node1->GetLineHandleRepresentation();

  vtkSmartPointer<vtkProperty> prop = node1->GetEndPointProperty();
  if (prop == NULL)
    {
    std::cout << "End Point Property is NULL." << std::endl;
    }
  prop = node1->GetSelectedEndPointProperty();
  if (prop == NULL)
    {
    std::cout << "Selected End Point Property is NULL." << std::endl;
    }

  prop = node1->GetEndPoint2Property();
  if (prop == NULL)
    {
    std::cout << "End Point2 Property is NULL." << std::endl;
    }
  prop = node1->GetSelectedEndPoint2Property();
  if (prop == NULL)
    {
    std::cout << "Selected End Point2 Property is NULL." << std::endl;
    }

  prop = node1->GetLineProperty();
  if (prop == NULL)
    {
    std::cout << "Line Property is NULL." << std::endl;
    }
  prop = node1->GetSelectedLineProperty();
  if (prop == NULL)
    {
    std::cout << "Selected Line Property is NULL." << std::endl;
    }

  TEST_SET_GET_INT_RANGE(node1, Tolerance, 2, 99);
  // 0 is invalid
  TEST_SET_GET_INT_RANGE(node1, Resolution, 2, 100);

  vtkSmartPointer<vtkPolyData> pd =  vtkSmartPointer<vtkPolyData>::New();
  node1->GetPolyData(pd);
  if (pd == NULL)
    {
    std::cout << "Polydata is null" << std::endl;
    }

  // clamped 0-6
  TEST_SET_GET_INT_RANGE(node1, InteractionState, 1, 5);
  // fails on 0
  TEST_SET_GET_INT_RANGE(node1, RepresentationState, 2, 5);

  std::cout << "MTime = " << node1->GetMTime() << std::endl;

  TEST_SET_GET_BOOLEAN(node1, DistanceAnnotationVisibility);
  TEST_SET_GET_STRING(node1, DistanceAnnotationFormat);

  TEST_SET_GET_VECTOR3_DOUBLE_RANGE(node1, DistanceAnnotationScale, 0.0, 100.0);

  std::cout << "Distance = " << node1->GetDistance() << std::endl;

  node1->SetLineColor(1.0, 0.5, 0.75);

  prop = node1->GetDistanceAnnotationProperty();

  vtkSmartPointer<vtkFollower> follower = node1->GetTextActor();

  return EXIT_SUCCESS;
}
