#include "vtkBiDimensionalRepresentation2D.h"

#include <cstdlib>
#include <iostream>

#include "vtkImageData.h"
#include "vtkTextProperty.h"
#include "vtkProperty2D.h"

#include "WidgetTestingMacros.h"

int vtkBiDimensionalRepresentation2DTest1(int , char * [] )
{
  vtkSmartPointer< vtkBiDimensionalRepresentation2D > node1 = vtkSmartPointer< vtkBiDimensionalRepresentation2D >::New();

  EXERCISE_BASIC_REPRESENTATION_METHODS(vtkBiDimensionalRepresentation2D, node1);

  double pos[3] = {55.0, 66.6, 77.9};
  double p[3];


  // world position
  node1->SetPoint1WorldPosition(pos);
  node1->GetPoint1WorldPosition(p);
  if (p[0] != pos[0] ||
      p[1] != pos[1] ||
      p[2] != pos[2])
  {
    std::cerr << "Failure in Get/Set Point1WorldPosition, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << ", instead got " << p[0] << ", " << p[1] << ", " << p[2]  << std::endl;
    return EXIT_FAILURE;
  }

  pos[2] = 99.9;
  node1->SetPoint2WorldPosition(pos);
  node1->GetPoint2WorldPosition(p);
  if (p[0] != pos[0] ||
      p[1] != pos[1] ||
      p[2] != pos[2])
  {
    std::cerr << "Failure in Get/Set Point2WorldPosition, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << ", instead got " << p[0] << ", " << p[1] << ", " << p[2]  << std::endl;
    return EXIT_FAILURE;
  }

  pos[2] = -88.9;
  node1->SetPoint3WorldPosition(pos);
  node1->GetPoint3WorldPosition(p);
  if (p[0] != pos[0] ||
      p[1] != pos[1] ||
      p[2] != pos[2])
  {
    std::cerr << "Failure in Get/Set Point3WorldPosition, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << ", instead got " << p[0] << ", " << p[1] << ", " << p[2]  << std::endl;
    return EXIT_FAILURE;
  }

  pos[0] = -44.9;
  node1->SetPoint4WorldPosition(pos);
  node1->GetPoint4WorldPosition(p);
  if (p[0] != pos[0] ||
      p[1] != pos[1] ||
      p[2] != pos[2])
  {
    std::cerr << "Failure in Get/Set Point4WorldPosition, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << ", instead got " << p[0] << ", " << p[1] << ", " << p[2]  << std::endl;
    return EXIT_FAILURE;
  }


  // display position
  node1->SetPoint1DisplayPosition(pos);
  node1->GetPoint1DisplayPosition(p);
  // only compare first two points in vector, last is always zero
  if (p[0] != pos[0] ||
      p[1] != pos[1] ||
      p[2] != 0.0)
  {
    std::cerr << "Failure in Get/Set Point1DisplayPosition, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << ", instead got " << p[0] << ", " << p[1] << ", " << p[2]  << std::endl;
    return EXIT_FAILURE;
  }

  pos[1] = 99.9;
  node1->SetPoint2DisplayPosition(pos);
  node1->GetPoint2DisplayPosition(p);
  if (p[0] != pos[0] ||
      p[1] != pos[1] ||
      p[2] != 0.0)
  {
    std::cerr << "Failure in Get/Set Point2DisplayPosition, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << ", instead got " << p[0] << ", " << p[1] << ", " << p[2]  << std::endl;
    return EXIT_FAILURE;
  }

  pos[1] = -88.9;
  node1->SetPoint3DisplayPosition(pos);
  node1->GetPoint3DisplayPosition(p);
  if (p[0] != pos[0] ||
      p[1] != pos[1] ||
      p[2] != 0.0)
  {
    std::cerr << "Failure in Get/Set Point3DisplayPosition, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << ", instead got " << p[0] << ", " << p[1] << ", " << p[2]  << std::endl;
    return EXIT_FAILURE;
  }

  pos[0] = -44.9;
  node1->SetPoint4DisplayPosition(pos);
  node1->GetPoint4DisplayPosition(p);
  if (p[0] != pos[0] ||
      p[1] != pos[1] ||
      p[2] != 0.0)
  {
    std::cerr << "Failure in Get/Set Point4DisplayPosition, expected " << pos[0] << ", " << pos[1] << ", " << pos[2] << ", instead got " << p[0] << ", " << p[1] << ", " << p[2]  << std::endl;
    return EXIT_FAILURE;
  }

  TEST_SET_GET_BOOLEAN(node1, Line1Visibility);
  TEST_SET_GET_BOOLEAN(node1, Line2Visibility);

  vtkSmartPointer<vtkHandleRepresentation> hRep;
  hRep = node1->GetPoint1Representation();
  hRep = node1->GetPoint2Representation();
  hRep = node1->GetPoint4Representation();
  hRep = node1->GetPoint4Representation();

  vtkSmartPointer<vtkProperty2D> prop2d;
  prop2d = node1->GetLineProperty();
  prop2d = node1->GetSelectedLineProperty();

  vtkSmartPointer<vtkTextProperty> textProp;
  textProp = node1->GetTextProperty();

  TEST_SET_GET_INT_RANGE(node1, Tolerance, 2, 99);


  std::cout << "Length 1 = " << node1->GetLength1() << std::endl;
  std::cout << "Length 2 = " << node1->GetLength2() << std::endl;

  TEST_SET_GET_STRING( node1, LabelFormat);

  double e[2] = {10.0, 8.0};
  node1->Point2WidgetInteraction(e);
  e[1] = 7.0;
  //node1->Point3WidgetInteraction(e);

  TEST_SET_GET_BOOLEAN(node1, ShowLabelAboveWidget);

  TEST_SET_GET_INT_RANGE(node1, ID, 1, 10000);

  std::cout << "LabelText = " << (node1->GetLabelText() ==  NULL ? "NULL" : node1->GetLabelText()) << std::endl;
  double *labelPos = node1->GetLabelPosition();
  if (labelPos)
  {
    std::cout << "LabelPosition: " << labelPos[0] << ", " << labelPos[1] << ", " << labelPos[2] << std::endl;
  }
  else
  {
    std::cout << "LabelPosition is null" << std::endl;
  }
  double labelPosition[3];
  node1->GetLabelPosition(labelPosition);
  std::cout << "LabelPosition [3]: " << labelPosition[0] << ", " << labelPosition[1] << ", " << labelPosition[2] << std::endl;
//  node1->GetWorldLabelPosition(labelPosition);
//  std::cout << "WorldLabelPosition: " << labelPosition[0] << ", " << labelPosition[1] << ", " << labelPosition[2] << std::endl;
  return EXIT_SUCCESS;
}
