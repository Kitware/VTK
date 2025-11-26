// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "WidgetTestingMacros.h"

#include "vtkLineRepresentation.h"
#include "vtkMultiLineRepresentation.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"

#include <iostream>

int TestMultiLineRepresentation(int, char*[])
{
  vtkNew<vtkMultiLineRepresentation> node1;

  EXERCISE_BASIC_REPRESENTATION_METHODS(vtkMultiLineRepresentation, node1);

  // point 1 world
  for (int i = 0; i < 4; i++)
  {
    double pos[3] = { -100.0 + i, 0.0, 99.9 + i };
    node1->GetLineRepresentation(i)->SetPoint1WorldPosition(pos);

    double* posptr = node1->GetLineRepresentation(i)->GetPoint1WorldPosition();

    if (!posptr)
    {
      std::cerr << "Error in get double * for Point1WorldPosition, null pointer returned."
                << std::endl;
      return EXIT_FAILURE;
    }
    else if (posptr[0] != pos[0] || posptr[1] != pos[1] || posptr[2] != pos[2])
    {
      std::cerr << "Error in double * Set/Get Point1WorldPosition, expected " << pos[0] << ", "
                << pos[1] << ", " << pos[2] << " but got " << posptr[0] << ", " << posptr[1] << ", "
                << posptr[2] << std::endl;
      return EXIT_FAILURE;
    }

    double pos2[3];
    node1->GetLineRepresentation(i)->GetPoint1WorldPosition(pos2);
    if (pos2[0] != pos[0] || pos2[1] != pos[1] || pos2[2] != pos[2])
    {
      std::cerr << "Error in Set/Get Point1WorldPosition, expected " << pos[0] << ", " << pos[1]
                << ", " << pos[2] << " but got " << pos2[0] << ", " << pos2[1] << ", " << pos2[2]
                << std::endl;
      return EXIT_FAILURE;
    }

    // point 2 world

    pos[1] = 77.0 + i;
    node1->GetLineRepresentation(i)->SetPoint2WorldPosition(pos);
    posptr = node1->GetLineRepresentation(i)->GetPoint2WorldPosition();
    if (!posptr)
    {
      std::cerr << "Error in get double * for Point2WorldPosition, null pointer returned."
                << std::endl;
      return EXIT_FAILURE;
    }
    else if (posptr[0] != pos[0] || posptr[1] != pos[1] || posptr[2] != pos[2])
    {
      std::cerr << "Error in double * Set/Get Point2WorldPosition, expected " << pos[0] << ", "
                << pos[1] << ", " << pos[2] << " but got " << posptr[0] << ", " << posptr[1] << ", "
                << posptr[2] << std::endl;
      return EXIT_FAILURE;
    }

    node1->GetLineRepresentation(i)->GetPoint2WorldPosition(pos2);
    if (pos2[0] != pos[0] || pos2[1] != pos[1] || pos2[2] != pos[2])
    {
      std::cerr << "Error in Set/Get Point2WorldPosition, expected " << pos[0] << ", " << pos[1]
                << ", " << pos[2] << " but got " << pos2[0] << ", " << pos2[1] << ", " << pos2[2]
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  TEST_SET_GET_INT_RANGE(node1, Tolerance, 2, 99);
  // 0 is invalid
  TEST_SET_GET_INT_RANGE(node1, Resolution, 2, 100);

  vtkNew<vtkPolyData> pd;
  for (int i = 0; i < 4; i++)
  {
    node1->GetLineRepresentation(i)->BuildRepresentation();
    node1->GetLineRepresentation(i)->GetPolyData(pd);
    if (pd == nullptr)
    {
      std::cerr << "Polydata is null" << std::endl;
      return EXIT_FAILURE;
    }

    TEST_SET_GET_BOOLEAN(node1->GetLineRepresentation(i), DistanceAnnotationVisibility);
    TEST_SET_GET_STRING(node1->GetLineRepresentation(i), DistanceAnnotationFormat);

    TEST_SET_GET_VECTOR3_DOUBLE_RANGE(
      node1->GetLineRepresentation(i), DistanceAnnotationScale, 0.0, 100.0);

    if (node1->GetLineRepresentation(i)->GetDistance() != 77.0 + i)
    {
      std::cerr << "Error in Distance, expected " << 77.0 + i << " but got "
                << node1->GetLineRepresentation(i)->GetDistance() << std::endl;
      return EXIT_FAILURE;
    }
    node1->SetLineColor(1.0, 0.5, 0.75);
  }

  return EXIT_SUCCESS;
}
