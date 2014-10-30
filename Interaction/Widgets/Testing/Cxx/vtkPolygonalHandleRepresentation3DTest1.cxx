#include "vtkPolygonalHandleRepresentation3D.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkPolygonalHandleRepresentation3DTest1(int , char * [] )
{
  vtkSmartPointer< vtkPolygonalHandleRepresentation3D > node1 = vtkSmartPointer< vtkPolygonalHandleRepresentation3D >::New();

  EXERCISE_BASIC_ABSTRACT_POLYGONAL_HANDLE_REPRESENTATION3D_METHODS(vtkPolygonalHandleRepresentation3D, node1);


  TEST_SET_GET_VECTOR3_DOUBLE_RANGE(node1, Offset, -10.0, 10.0);

  return EXIT_SUCCESS;
}
