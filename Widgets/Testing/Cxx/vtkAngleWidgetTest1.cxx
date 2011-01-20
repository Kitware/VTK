#include "vtkAngleWidget.h"
#include "vtkAngleRepresentation2D.h"
#include "vtkAngleRepresentation3D.h"

#include <stdlib.h>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkAngleWidgetTest1(int , char * [] )
{
  vtkSmartPointer< vtkAngleWidget > node1 = vtkSmartPointer< vtkAngleWidget >::New();

  EXERCISE_BASIC_ABSTRACT_METHODS ( node1 );

  std::cout << "Angle Valid = " << node1->IsAngleValid() << std::endl;

  node1->SetProcessEvents(1);
  node1->SetProcessEvents(0);

  vtkSmartPointer<vtkAngleRepresentation2D> rep2d =  vtkSmartPointer<vtkAngleRepresentation2D>::New();
  node1->SetRepresentation(rep2d);

  vtkSmartPointer<vtkAngleRepresentation3D> rep3d =  vtkSmartPointer<vtkAngleRepresentation3D>::New();
  node1->SetRepresentation(rep3d);


  std::cout << "Can't get at WidgetState, CurrentHandle, subwidgets" <<std::endl;

  return EXIT_SUCCESS;
}
