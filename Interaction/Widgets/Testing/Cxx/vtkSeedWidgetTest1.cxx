#include "vtkSeedWidget.h"
#include "vtkSeedRepresentation.h"
#include "vtkHandleWidget.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"
#include "vtkPointHandleRepresentation2D.h"

int vtkSeedWidgetTest1(int , char * [] )
{
  vtkSmartPointer< vtkSeedWidget > node1 = vtkSmartPointer< vtkSeedWidget >::New();

  EXERCISE_BASIC_ABSTRACT_METHODS ( node1 );

  node1->SetProcessEvents(0);
  node1->SetProcessEvents(1);

  vtkSmartPointer<vtkSeedRepresentation> rep1 = vtkSmartPointer<vtkSeedRepresentation>::New();
  node1->SetRepresentation(rep1);

  node1->CompleteInteraction();
  node1->RestartInteraction();

  // have to create a handle rep before create new handle
  vtkSmartPointer<vtkPointHandleRepresentation2D> handle = vtkSmartPointer<vtkPointHandleRepresentation2D>::New();
  handle->GetProperty()->SetColor(1,0,0);
  rep1->SetHandleRepresentation(handle);


  vtkSmartPointer<vtkHandleWidget> handleWidget = node1->CreateNewHandle();
  if (handleWidget == NULL)
    {
    std::cerr << "Failed to CreateNewHandle." << std::endl;
    return EXIT_FAILURE;
    }
  vtkSmartPointer<vtkHandleWidget> handleWidget2 = node1->GetSeed(0);
  if (handleWidget2 != handleWidget)
    {
    std::cerr << "Failed to get seed 0 handle" << std::endl;
    return EXIT_FAILURE;
    }

  // try deleting a seed that doesn't exist
  node1->DeleteSeed(100);
  // now delete the one that we added
  node1->DeleteSeed(0);

  return EXIT_SUCCESS;
}
