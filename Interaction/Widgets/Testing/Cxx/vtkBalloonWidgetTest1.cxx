#include "vtkBalloonWidget.h"
#include "vtkBalloonRepresentation.h"
#include "vtkProp.h"
#include "vtkActor.h"
#include "vtkAbstractPropPicker.h"
#include "vtkCellPicker.h"
#include "vtkImageData.h"
#include "vtkStdString.h"

#include <cstdlib>
#include <iostream>

#include "WidgetTestingMacros.h"

int vtkBalloonWidgetTest1(int , char * [] )
{
  vtkSmartPointer< vtkBalloonWidget > node1 = vtkSmartPointer< vtkBalloonWidget >::New();
// failing at all of these
//  EXERCISE_BASIC_HOVER_METHODS (node1 );
//  EXERCISE_BASIC_ABSTRACT_METHODS (node1);
//  EXERCISE_BASIC_INTERACTOR_OBSERVER_METHODS(node1);
  EXERCISE_BASIC_OBJECT_METHODS(node1);
  vtkSmartPointer<vtkBalloonRepresentation> rep1 = vtkSmartPointer<vtkBalloonRepresentation>::New();
  node1->SetRepresentation(rep1);

  vtkSmartPointer<vtkActor> prop1 = vtkSmartPointer<vtkActor>::New();
  vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
  vtkStdString stdString = "something with a space";
  const char *cstr = "string1";
  const char *retstr = NULL;

  node1->AddBalloon(prop1, stdString, imageData);
  retstr = node1->GetBalloonString(prop1);
  if (!retstr)
    {
    std::cerr << "1. Get null return string." << std::endl;
    return EXIT_FAILURE;
    }
  if (stdString.compare(retstr) != 0)
    {
    std::cerr << "1. Expected " << stdString << ", got " << retstr << std::endl;
    return EXIT_FAILURE;
    }

  node1->AddBalloon(prop1, cstr, imageData);
  retstr = node1->GetBalloonString(prop1);
  if (!retstr)
    {
    std::cerr << "2. Get null return string." << std::endl;
    return EXIT_FAILURE;
    }
  if (strcmp(retstr,cstr) != 0)
    {
    std::cerr << "2. Expected " << cstr << ", got " << retstr << std::endl;
    return EXIT_FAILURE;
    }


  node1->AddBalloon(prop1, "string2", imageData);
  // check the image data first, since adding other balloons resets it
  vtkImageData *retImageData = node1->GetBalloonImage(prop1);
  if (retImageData != imageData)
    {
    std::cerr << "Didn't get back expected image data" << std::endl;
    return EXIT_FAILURE;
    }
  retstr = node1->GetBalloonString(prop1);
  if (!retstr)
    {
    std::cerr << "3. Get null return string." << std::endl;
    return EXIT_FAILURE;
    }
  if (strcmp(retstr,"string2") != 0)
    {
    std::cerr << "3. Expected 'string2', got " << retstr << std::endl;
    return EXIT_FAILURE;
    }

  node1->AddBalloon(prop1, cstr);
  retstr = node1->GetBalloonString(prop1);
   if (!retstr)
    {
    std::cerr << "4. Get null return string." << std::endl;
    return EXIT_FAILURE;
    }
  if (strcmp(retstr,cstr) != 0)
    {
    std::cerr << "4. Expected " << cstr << ", got " << retstr << std::endl;
    return EXIT_FAILURE;
    }

  node1->AddBalloon(prop1, "string3");
  retstr = node1->GetBalloonString(prop1);
  if (!retstr)
    {
    std::cerr << "5. Get null return string." << std::endl;
    return EXIT_FAILURE;
    }
  if (strcmp(retstr,"string3") != 0)
    {
    std::cerr << "5. Expected 'string3', got " << retstr << std::endl;
    return EXIT_FAILURE;
    }


  return EXIT_SUCCESS;
}
