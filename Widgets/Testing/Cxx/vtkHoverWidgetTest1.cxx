#include "vtkHoverWidget.h"

#include <stdlib.h>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkHoverWidgetTest1(int , char * [] )
{
  vtkSmartPointer< vtkHoverWidget > node1 = vtkSmartPointer< vtkHoverWidget >::New();

  EXERCISE_BASIC_HOVER_METHODS (node1 );

  return EXIT_SUCCESS;
}
