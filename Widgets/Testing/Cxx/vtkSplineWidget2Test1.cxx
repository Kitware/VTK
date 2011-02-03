#include "vtkSplineWidget2.h"
#include "vtkSplineRepresentation.h"

#include <stdlib.h>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkSplineWidget2Test1(int , char * [] )
{
  vtkSmartPointer< vtkSplineWidget2 > node1 = vtkSmartPointer< vtkSplineWidget2 >::New();

  EXERCISE_BASIC_ABSTRACT_METHODS ( node1 );

  vtkSmartPointer<vtkSplineRepresentation> rep1 = vtkSmartPointer<vtkSplineRepresentation>::New();
  node1->SetRepresentation(rep1);

  return EXIT_SUCCESS;
}
