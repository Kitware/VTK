#include "vtkCaptionWidget.h"
#include "vtkCaptionRepresentation.h"
#include "vtkCaptionActor2D.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkCaptionWidgetTest1(int , char * [] )
{
  vtkSmartPointer< vtkCaptionWidget > node1 = vtkSmartPointer< vtkCaptionWidget >::New();

  EXERCISE_BASIC_BORDER_METHODS (node1 );

  vtkSmartPointer<vtkCaptionRepresentation> rep1 = vtkSmartPointer<vtkCaptionRepresentation>::New();
  node1->SetRepresentation(rep1);

  vtkCaptionActor2D *captionActor = node1->GetCaptionActor2D();
  if (captionActor)
  {
    std::cout << "Caption actor is not null" << std::endl;
  }
  else
  {
    std::cout << "Caption actor is null" << std::endl;
  }
  vtkSmartPointer<vtkCaptionActor2D> captionActor2 = vtkSmartPointer<vtkCaptionActor2D>::New();
  node1->SetCaptionActor2D(captionActor2);
  if (node1->GetCaptionActor2D() != captionActor2)
  {
    std::cerr << "Failed to get back expected caption actor" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
