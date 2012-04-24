/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPlane.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMath.h"
#include "vtkPlane.h"
#include "vtkSmartPointer.h"

#include <limits>

#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

template<class A>
bool fuzzyCompare1D(A a, A b)
{
  return ABS(a - b) < std::numeric_limits<A>::epsilon();
}

template<class A>
bool fuzzyCompare3D(A a[3], A b[3])
{
  return fuzzyCompare1D(a[0], b[0]) &&
         fuzzyCompare1D(a[1], b[1]) &&
         fuzzyCompare1D(a[2], b[2]);
}

int TestPlane(int,char *[])
{
  // Test ProjectVector (vector is out of plane)
  {
  vtkSmartPointer<vtkPlane> plane =
    vtkSmartPointer<vtkPlane>::New();
  plane->SetOrigin(0.0, 0.0, 0.0);
  plane->SetNormal(0,0,1);

  std::cout << "Testing ProjectVector" << std::endl;
  double v[3] = {1,2,3};
  double projection[3];
  double correct[3] = {1., 2., 0};
  plane->ProjectVector(v, projection);
  if(!fuzzyCompare3D(projection,correct))
    {
    std::cerr << "ProjectVector failed! Should be (1., 2., 0) but it is ("
                  << projection[0] << " " << projection[1] << " " << projection[2] << ")" << std::endl;
    return EXIT_FAILURE;
    }
  }

  // Test ProjectVector where vector is already in plane
  {
  vtkSmartPointer<vtkPlane> plane =
    vtkSmartPointer<vtkPlane>::New();
  plane->SetOrigin(0.0, 0.0, 0.0);
  plane->SetNormal(0,0,1);

  std::cout << "Testing ProjectVector" << std::endl;
  double v[3] = {1,2,0};
  double projection[3];
  double correct[3] = {1., 2., 0};
  plane->ProjectVector(v, projection);
  if(!fuzzyCompare3D(projection,correct))
    {
    std::cerr << "ProjectVector failed! Should be (1., 2., 0) but it is ("
                  << projection[0] << " " << projection[1] << " " << projection[2] << ")" << std::endl;
    return EXIT_FAILURE;
    }
  }

  // Test ProjectVector where vector is orthogonal to plane
  {
  vtkSmartPointer<vtkPlane> plane =
    vtkSmartPointer<vtkPlane>::New();
  plane->SetOrigin(0.0, 0.0, 0.0);
  plane->SetNormal(0,0,1);

  std::cout << "Testing ProjectVector" << std::endl;
  double v[3] = {0,0,1};
  double projection[3];
  double correct[3] = {0., 0., 0.};
  plane->ProjectVector(v, projection);
  if(!fuzzyCompare3D(projection,correct))
    {
    std::cerr << "ProjectVector failed! Should be (0., 0., 0) but it is ("
                  << projection[0] << " " << projection[1] << " " << projection[2] << ")" << std::endl;
    return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
