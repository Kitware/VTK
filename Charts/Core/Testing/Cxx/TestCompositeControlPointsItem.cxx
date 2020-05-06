/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCompositeControlPointsItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Charts includes
#include "vtkCompositeControlPointsItem.h"

// STD includes
#include <iostream>

//------------------------------------------------------------------------------
int TestCompositeControlPointsItem(int, char*[])
{
  vtkNew<vtkCompositeControlPointsItem> controlPoints;

  double point0[4] = { 0., 0., 0.5, 0. };
  double point1[4] = { 50., 0.2, 0.5, 0. };
  double point2[4] = { 50., 0.8, 0.5, 0. };
  double point3[4] = { 100., 1., 0.5, 0. };

  controlPoints->AddPoint(point0);
  controlPoints->AddPoint(point1);
  controlPoints->AddPoint(point2);
  controlPoints->AddPoint(point3);

  controlPoints->GetControlPoint(0, point0);
  controlPoints->GetControlPoint(1, point1);
  controlPoints->GetControlPoint(2, point2);
  controlPoints->GetControlPoint(3, point3);
  if (point0[0] != 0. || point1[0] != 50. || point2[0] != 50. || point3[0] != 100.)
  {
    std::cerr << "vtkCompositeControlPointsItem failed, wrong pos: " << point0[0] << ", "
              << point1[0] << ", " << point2[0] << ", " << point3[0] << std::endl;
    return EXIT_FAILURE;
  }

  // Make sure duplicate point can be removed correctly
  controlPoints->RemovePoint(2);
  controlPoints->GetControlPoint(1, point1);
  if (point1[0] != 50. || point1[1] != 0.2)
  {
    std::cerr << "vtkCompositeControlPointsItem::RemovePoint"
              << "failed to delete duplicated point correctly" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
