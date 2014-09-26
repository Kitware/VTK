#include "vtkBiDimensionalWidget.h"
#include "vtkBiDimensionalRepresentation2D.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkBiDimensionalWidgetTest1(int , char * [] )
{
  vtkSmartPointer< vtkBiDimensionalWidget > node1 = vtkSmartPointer< vtkBiDimensionalWidget >::New();

  EXERCISE_BASIC_ABSTRACT_METHODS ( node1 );

  std::cout << "Measure Valid = " << node1->IsMeasureValid() << std::endl;

  node1->SetProcessEvents(1);
  node1->SetProcessEvents(0);

   vtkSmartPointer<vtkBiDimensionalRepresentation2D> rep1 = vtkSmartPointer<vtkBiDimensionalRepresentation2D>::New();
  node1->SetRepresentation(rep1);

  return EXIT_SUCCESS;
}
