#include "vtkConstrainedPointHandleRepresentation.h"

#include <stdlib.h>
#include <iostream>

#include "WidgetTestingMacros.h"
#include "vtkPolyData.h"

#include <vtkProperty.h>
#include <vtkPlane.h>
#include <vtkPlanes.h>
#include <vtkPlaneCollection.h>

int vtkConstrainedPointHandleRepresentationTest1(int , char * [] )
{
  vtkSmartPointer< vtkConstrainedPointHandleRepresentation > node1 = vtkSmartPointer< vtkConstrainedPointHandleRepresentation >::New();

  EXERCISE_BASIC_HANDLE_REPRESENTATION_METHODS(vtkConstrainedPointHandleRepresentation, node1);

  vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
  node1->SetCursorShape(pd);
  vtkSmartPointer<vtkPolyData> pd2 = node1->GetCursorShape();
  if (!pd2 ||
      pd2 != pd)
    {
    std::cerr << "Error in Set/Get cursor shape." << std::endl;
    return EXIT_FAILURE;
    }

  vtkSmartPointer<vtkPolyData> pd3 = vtkSmartPointer<vtkPolyData>::New();
  node1->SetActiveCursorShape(pd);
  vtkSmartPointer<vtkPolyData> pd4 = node1->GetActiveCursorShape();
  if (!pd4 ||
      pd4 != pd3)
    {
    std::cerr << "Error in Set/Get active cursor shape." << std::endl;
    return EXIT_FAILURE;
    }

  // constrained 0-3
  TEST_SET_GET_INT_RANGE(node1, ProjectionNormal, 1, 2);
  node1->SetProjectionNormalToXAxis();
  node1->SetProjectionNormalToYAxis();
  node1->SetProjectionNormalToZAxis();
  node1->SetProjectionNormalToOblique();

  vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
  node1->SetObliquePlane(plane);
  vtkSmartPointer<vtkPlane> plane2 = node1->GetObliquePlane();
  if (!plane2 ||
      plane2 != plane)
    {
    std::cerr << "Error in Set/Get oblique plane." << std::endl;
    return EXIT_FAILURE;
    }

  TEST_SET_GET_DOUBLE_RANGE(node1, ProjectionPosition, -10.0, 10.0);


  vtkSmartPointer<vtkPlane> bplane = vtkSmartPointer<vtkPlane>::New();
  vtkSmartPointer<vtkPlane> bplane2 = vtkSmartPointer<vtkPlane>::New();
  node1->AddBoundingPlane(bplane);
  node1->AddBoundingPlane(bplane2);
  node1->RemoveBoundingPlane(bplane);
  node1->RemoveAllBoundingPlanes();

  vtkSmartPointer<vtkPlaneCollection> planeCol = vtkSmartPointer<vtkPlaneCollection>::New();
  node1->SetBoundingPlanes(planeCol);
  vtkSmartPointer<vtkPlaneCollection> planeCol2 = node1->GetBoundingPlanes();
  if (!planeCol2 ||
      planeCol2 != planeCol)
    {
    std::cerr << "Error in Set/Get bounding planes." << std::endl;
    return EXIT_FAILURE;
    }

  vtkSmartPointer<vtkPlanes> bplanes = vtkSmartPointer<vtkPlanes>::New();
  node1->SetBoundingPlanes(bplanes);

  // test Set/GetPosition, in display coords, so only x,y are used
  double pos[3] = {10.0, 11.0, -12.0};
  double *pos2 = NULL;
  node1->SetPosition(pos);
  pos2 = node1->GetPosition();
  if (pos2 == NULL)
    {
    std::cerr << "Failure in Get/Set Position pos,  null pointer." << std::endl;
    return EXIT_FAILURE;
    }
  else if (pos2[0] != pos[0] ||
           pos2[1] != pos[1])
    {
    std::cerr << "Failure in Get/Set Position pos, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] <<", instead got " << pos2[0] << ", " << pos2[1] << ", " << pos2[2]  << std::endl;
    return EXIT_FAILURE;
    }
  else
    {
    std::cout << "Set Position to "  << pos2[0] << ", " << pos2[1] << ", " << pos2[2]  << std::endl;
    }
  pos[0] = 12.0;
  node1->SetPosition(pos[0], pos[1], pos[2]);
  pos2 = node1->GetPosition();
   if (pos2 == NULL)
    {
    std::cerr << "Failure in Get/Set Position pos,  null pointer." << std::endl;
    return EXIT_FAILURE;
    }
  else if (pos2[0] != pos[0] ||
           pos2[1] != pos[1])
    {
    std::cerr << "Failure in Get/Set Position pos, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] <<", instead got " << pos2[0] << ", " << pos2[1] << ", " << pos2[2]  << std::endl;
    return EXIT_FAILURE;
    }
  else
    {
    std::cout << "Set Position to "  << pos2[0] << ", " << pos2[1] << ", " << pos2[2]  << std::endl;
    }

   pos[0] -= 1.0;
   node1->SetPosition(pos[0], pos[1], pos[2]);
   double pos3[3];
   node1->GetPosition(pos3);
   if (pos3[0] != pos[0] ||
       pos3[1] != pos[1])
    {
    std::cerr << "Failure in Get/Set Position pos, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] <<", instead got " << pos3[0] << ", " << pos3[1] << ", " << pos3[2]  << std::endl;
    return EXIT_FAILURE;
    }
  else
    {
    std::cout << "Set Position to "  << pos3[0] << ", " << pos3[1] << ", " << pos3[2]  << std::endl;
    }

   // Properties
  vtkSmartPointer<vtkProperty> prop1 = vtkSmartPointer<vtkProperty>::New();
  double colour[3] = {0.2, 0.3, 0.4};
  prop1->SetColor(colour);
  //node1->SetProperty(prop1);
  vtkSmartPointer<vtkProperty> prop = node1->GetProperty();
  if (!prop)
    {
    std::cerr << "Got null property back after setting it!" << std::endl;
    return EXIT_FAILURE;
    }
  double *col = prop->GetColor();
  if (!col)
    {
    std::cerr << "Got null colour back!" << std::endl;
    return EXIT_FAILURE;
    }
  if (col[0] != colour[0] ||
      col[1] != colour[1] ||
      col[2] != colour[2])
    {
    std::cerr << "Got wrong colour back after setting it! Expected " << colour[0] << ", " << colour[1] << ", " << colour[2] << ", but got " << col[0] << ", " << col[1] << ", " << col[2] << std::endl;
    return EXIT_FAILURE;
    }

  vtkSmartPointer<vtkProperty> prop2 = vtkSmartPointer<vtkProperty>::New();
  colour[0] += 0.1;
  colour[2] += 0.1;
  colour[2] += 0.1;
  prop2->SetColor(colour);
//  node1->SetSelectedProperty(prop2);
  prop = node1->GetSelectedProperty();
  if (!prop)
    {
    std::cerr << "Got null selected property back after setting it!" << std::endl;
    return EXIT_FAILURE;
    }
  col = prop->GetColor();
  if (!col)
    {
    std::cerr << "Got null selected colour back!" << std::endl;
    return EXIT_FAILURE;
    }
  if (col[0] != colour[0] ||
      col[1] != colour[1] ||
      col[2] != colour[2])
    {
    std::cerr << "Got wrong selected colour back after setting it! Expected " << colour[0] << ", " << colour[1] << ", " << colour[2] << ", but got " << col[0] << ", " << col[1] << ", " << col[2] << std::endl;
    return EXIT_FAILURE;
    }

  prop = node1->GetActiveProperty();
  colour[0] += 0.1;
  colour[2] += 0.1;
  colour[2] += 0.1;
  prop->SetColor(colour);

  return EXIT_SUCCESS;
}
