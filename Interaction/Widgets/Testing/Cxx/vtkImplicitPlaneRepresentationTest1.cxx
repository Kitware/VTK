#include "vtkImplicitPlaneRepresentation.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkPlane.h>
#include <vtkProperty.h>

int vtkImplicitPlaneRepresentationTest1(int , char * [] )
{
  vtkSmartPointer< vtkImplicitPlaneRepresentation > node1 = vtkSmartPointer< vtkImplicitPlaneRepresentation >::New();

  EXERCISE_BASIC_IMPLICIT_PLANE_REPRESENTATION_METHODS(vtkImplicitPlaneRepresentation, node1);

  vtkSmartPointer<vtkPolyData> pd;
  node1->GetPolyData(pd);
  if (pd == NULL)
    {
    std::cout << "Polydata is null" << std::endl;
    }

  vtkSmartPointer<vtkPolyDataAlgorithm> pda = node1->GetPolyDataAlgorithm();
  if (pda == NULL)
    {
    std::cout << "Polydata algorithm is null" << std::endl;
    }

  vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
  node1->GetPlane(plane);
  if (!plane)
    {
    std::cout << "Plane is null" << std::endl;
    }

  node1->UpdatePlacement();


  vtkSmartPointer<vtkProperty> prop = node1->GetNormalProperty();
  if (prop == NULL)
    {
    std::cout << "Normal Property is NULL." << std::endl;
    }
  prop = node1->GetSelectedNormalProperty();
  if (prop == NULL)
    {
    std::cout << "Selected Normal Property is NULL." << std::endl;
    }

  prop = node1->GetPlaneProperty();
  if (prop == NULL)
    {
    std::cout << "Plane Property is NULL." << std::endl;
    }
  prop = node1->GetSelectedPlaneProperty();
  if (prop == NULL)
    {
    std::cout << "Selected Plane Property is NULL." << std::endl;
    }

  prop = node1->GetOutlineProperty();
  if (prop == NULL)
    {
    std::cout << "Outline Property is NULL." << std::endl;
    }
  prop = node1->GetSelectedOutlineProperty();
  if (prop == NULL)
    {
    std::cout << "Selected Outline Property is NULL." << std::endl;
    }

  prop = node1->GetEdgesProperty();
  if (prop == NULL)
    {
    std::cout << "Edges Property is NULL." << std::endl;
    }

  // clamped 0-7
  TEST_SET_GET_INT_RANGE(node1, InteractionState, 1, 6);

  TEST_SET_GET_INT_RANGE(node1, RepresentationState, 0, 5);
  return EXIT_SUCCESS;
}
