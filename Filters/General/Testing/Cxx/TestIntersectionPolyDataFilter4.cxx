/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIntersectionPolyDataFilter4.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include <vtkIntersectionPolyDataFilter.h>

#include <vtkNew.h>
#include <vtkSphereSource.h>
#include <vtkTriangleFilter.h>

// This test exercises the conditions that previously led to an out-of-bounds
// memory access when computing the intersection between two surfaces, at least
// one of which was not entirely enclosed (the sphere ending at Theta=305 below).
int TestIntersectionPolyDataFilter4(int, char*[])
{
  vtkNew<vtkSphereSource> sphere1;
  sphere1->SetStartTheta(0.0);
  sphere1->SetEndTheta(305.0);
  vtkNew<vtkTriangleFilter> triangle1;
  triangle1->SetInputConnection(sphere1->GetOutputPort());

  vtkNew<vtkSphereSource> sphere2;
  sphere2->SetCenter(0.2, 0.0, 0.0);
  vtkNew<vtkTriangleFilter> triangle2;
  triangle2->SetInputConnection(sphere2->GetOutputPort());

  vtkNew<vtkIntersectionPolyDataFilter> interFilter;
  interFilter->SetInputConnection(0, triangle1->GetOutputPort());
  interFilter->SetInputConnection(1, triangle2->GetOutputPort());
  interFilter->SplitFirstOutputOn();
  interFilter->SplitSecondOutputOn();
  interFilter->Update();

  return EXIT_SUCCESS;
}
