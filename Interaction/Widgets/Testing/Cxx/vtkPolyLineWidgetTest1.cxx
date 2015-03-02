#include "vtkPolyLineWidget.h"
#include "vtkPolyLineRepresentation.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkPolyLineWidgetTest1(int , char * [] )
{
  vtkSmartPointer< vtkPolyLineWidget > node1 = vtkSmartPointer< vtkPolyLineWidget >::New();

  EXERCISE_BASIC_ABSTRACT_METHODS ( node1 );

  vtkSmartPointer<vtkPolyLineRepresentation> rep1 = vtkSmartPointer<vtkPolyLineRepresentation>::New();
  node1->SetRepresentation(rep1);

  EXERCISE_BASIC_INTERACTOR_OBSERVER_METHODS( node1 );

  return EXIT_SUCCESS;
}
