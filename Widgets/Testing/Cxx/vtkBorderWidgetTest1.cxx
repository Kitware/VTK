#include "vtkBorderWidget.h"
#include "vtkBorderRepresentation.h"

#include <stdlib.h>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkBorderWidgetTest1(int , char * [] )
{
  vtkSmartPointer< vtkBorderWidget > node1 = vtkSmartPointer< vtkBorderWidget >::New();

  EXERCISE_BASIC_BORDER_METHODS (node1 );

  vtkSmartPointer<vtkBorderRepresentation> rep1 = vtkSmartPointer<vtkBorderRepresentation>::New();
  node1->SetRepresentation(rep1);

  return EXIT_SUCCESS;
}
