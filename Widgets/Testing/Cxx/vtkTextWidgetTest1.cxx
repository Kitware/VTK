#include "vtkTextWidget.h"
#include "vtkTextRepresentation.h"
#include "vtkTextActor.h"

#include <stdlib.h>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkTextWidgetTest1(int , char * [] )
{
  vtkSmartPointer< vtkTextWidget > node1 = vtkSmartPointer< vtkTextWidget >::New();

  EXERCISE_BASIC_BORDER_METHODS (node1 );

  vtkSmartPointer<vtkTextRepresentation> rep1 = vtkSmartPointer<vtkTextRepresentation>::New();
  node1->SetRepresentation(rep1);

  vtkTextActor *textActor = node1->GetTextActor();
  if (textActor)
    {
    std::cout << "Text actor is not null" << std::endl;
    }
  else
    {
    std::cout << "Text actor is null" << std::endl;
    }
  vtkSmartPointer<vtkTextActor> textActor2 = vtkSmartPointer<vtkTextActor>::New();
  node1->SetTextActor(textActor2);
  if (node1->GetTextActor() != textActor2)
    {
    std::cerr << "Failed to get back expected text actor" << std::endl;
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
