#include "vtkLineWidget2.h"
#include "vtkLineRepresentation.h"

#include <stdlib.h>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkLineWidget2Test1(int , char * [] )
{
  vtkSmartPointer< vtkLineWidget2 > node1 = vtkSmartPointer< vtkLineWidget2 >::New();

  EXERCISE_BASIC_ABSTRACT_METHODS ( node1 );

  node1->SetProcessEvents(0);
  node1->SetProcessEvents(1);

  vtkSmartPointer<vtkLineRepresentation> rep1 = vtkSmartPointer<vtkLineRepresentation>::New();
  node1->SetRepresentation(rep1);

  return EXIT_SUCCESS;
}
