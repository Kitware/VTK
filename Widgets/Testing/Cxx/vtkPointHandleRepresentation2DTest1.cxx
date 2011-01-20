#include "vtkPointHandleRepresentation2D.h"

#include <stdlib.h>
#include <iostream>

#include "WidgetTestingMacros.h"
#include "vtkPolyData.h"

#include <vtkProperty2D.h>

int vtkPointHandleRepresentation2DTest1(int , char * [] )
{
  vtkSmartPointer< vtkPointHandleRepresentation2D > node1 = vtkSmartPointer< vtkPointHandleRepresentation2D >::New();

  EXERCISE_BASIC_HANDLE_REPRESENTATION_METHODS(vtkPointHandleRepresentation2D, node1);

  std::cout << "Done exercise basic handl representation methods" << std::endl;
  if (node1->GetBounds() == NULL)
    {
    std::cout << "Bounds are null." << std::endl;
    }

  vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
  node1->SetCursorShape(pd);
  vtkSmartPointer<vtkPolyData> pd2 = node1->GetCursorShape();
  if (!pd2 ||
      pd2 != pd)
    {
    std::cerr << "Error in Set/Get cursor shape." << std::endl;
    return EXIT_FAILURE;
    }

  vtkSmartPointer<vtkProperty2D> prop1 = vtkSmartPointer<vtkProperty2D>::New();
  double colour[3] = {0.2, 0.3, 0.4};
  prop1->SetColor(colour);
  node1->SetProperty(prop1);
  vtkSmartPointer<vtkProperty2D> prop = node1->GetProperty();
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

  vtkSmartPointer<vtkProperty2D> prop2 = vtkSmartPointer<vtkProperty2D>::New();
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

  return EXIT_SUCCESS;
}
