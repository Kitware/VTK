#include "vtkOrientedPolygonalHandleRepresentation3D.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

#include <vtkProperty2D.h>

int vtkOrientedPolygonalHandleRepresentation3DTest1(int , char * [] )
{
  vtkSmartPointer< vtkOrientedPolygonalHandleRepresentation3D > node1 = vtkSmartPointer< vtkOrientedPolygonalHandleRepresentation3D >::New();

  EXERCISE_BASIC_ABSTRACT_POLYGONAL_HANDLE_REPRESENTATION3D_METHODS(vtkOrientedPolygonalHandleRepresentation3D, node1);


  return EXIT_SUCCESS;
}
