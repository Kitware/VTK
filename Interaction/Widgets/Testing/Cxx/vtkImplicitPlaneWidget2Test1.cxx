#include "vtkImplicitPlaneWidget2.h"
#include "vtkImplicitPlaneRepresentation.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkImplicitPlaneWidget2Test1(int , char * [] )
{
  vtkSmartPointer< vtkImplicitPlaneWidget2 > node1 = vtkSmartPointer< vtkImplicitPlaneWidget2 >::New();

  EXERCISE_BASIC_ABSTRACT_METHODS ( node1 );

  vtkSmartPointer<vtkImplicitPlaneRepresentation> rep1 = vtkSmartPointer<vtkImplicitPlaneRepresentation>::New();
  node1->SetRepresentation(rep1);

  return EXIT_SUCCESS;
}
