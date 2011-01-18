#include "vtkSphereHandleRepresentation.h"

#include <stdlib.h>
#include <iostream>

#include "WidgetTestingMacros.h"

#include <vtkPointHandleRepresentation3D.h>
#include <vtkProperty.h>

int vtkSphereHandleRepresentationTest1(int , char * [] )
{
  vtkSmartPointer< vtkSphereHandleRepresentation > node1 = vtkSmartPointer< vtkSphereHandleRepresentation >::New();

  EXERCISE_BASIC_HANDLE_REPRESENTATION_METHODS(vtkSphereHandleRepresentation, node1);

  std::cout << "Done basic handle rep methods." << std::endl;

  TEST_SET_GET_BOOLEAN(node1, TranslationMode);

  TEST_SET_GET_DOUBLE_RANGE(node1, SphereRadius, 0.0, 100.0);

  vtkSmartPointer<vtkProperty> prop1 = vtkSmartPointer<vtkProperty>::New();
  double colour[3] = {0.2, 0.3, 0.4};
  prop1->SetColor(colour);
  node1->SetProperty(prop1);
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
  node1->SetSelectedProperty(prop2);
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

  // clamped 0-1
  TEST_SET_GET_DOUBLE_RANGE(node1, HotSpotSize, 0.1, 0.9);


  return EXIT_SUCCESS;
}
