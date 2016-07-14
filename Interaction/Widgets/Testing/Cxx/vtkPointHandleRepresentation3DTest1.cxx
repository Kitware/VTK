#include "vtkPointHandleRepresentation3D.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

#include "vtkProperty.h"

int vtkPointHandleRepresentation3DTest1(int , char * [] )
{
  vtkSmartPointer< vtkPointHandleRepresentation3D > node1 = vtkSmartPointer< vtkPointHandleRepresentation3D >::New();

  EXERCISE_BASIC_HANDLE_REPRESENTATION_METHODS(vtkPointHandleRepresentation3D, node1);

  TEST_SET_GET_INT_RANGE(node1, Outline, 0, 1);
  std::cout << "Outline = " << node1->GetOutline() << std::endl;
  node1->OutlineOn();
  node1->OutlineOff();

  TEST_SET_GET_INT_RANGE(node1, XShadows, 0, 1);
  std::cout << "XShadows = " << node1->GetXShadows() << std::endl;
  node1->XShadowsOn();
  node1->XShadowsOff();

  TEST_SET_GET_INT_RANGE(node1, YShadows, 0, 1);
  std::cout << "YShadows = " << node1->GetYShadows() << std::endl;
  node1->YShadowsOn();
  node1->YShadowsOff();

  TEST_SET_GET_INT_RANGE(node1, ZShadows, 0, 1);
  std::cout << "ZShadows = " << node1->GetZShadows() << std::endl;
  node1->ZShadowsOn();
  node1->ZShadowsOff();

  TEST_SET_GET_BOOLEAN(node1, TranslationMode);

  node1->AllOn();
  node1->AllOff();

  TEST_SET_GET_PROPERTY(object, Property);
  TEST_SET_GET_PROPERTY(object, SelectedProperty);
  /*
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
  */
  // clamped 0-1
  TEST_SET_GET_DOUBLE_RANGE(node1, HotSpotSize, 0.1, 0.9);

  TEST_SET_GET_DOUBLE_RANGE(node1, HandleSize, 1.0, 10.0);
  return EXIT_SUCCESS;
}
