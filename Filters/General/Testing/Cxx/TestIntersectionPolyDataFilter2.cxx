/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIntersectionPolyDataFilter2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include <vtkIntersectionPolyDataFilter.h>

#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>

int TestIntersectionPolyDataFilter2(int, char *[])
{
  // Set up two polydata representing two triangles that share a vertex
  int coplanar;
  double isectpt1[3], isectpt2[3];
  double thisCellTri[9] = {-30.125,
                           29.3125,
                           -27.1875,
                           -29.9375,
                           29.375,
                           -27.3125,
                           -30.0625,
                           28.5,
                           -27.25};
  double otherCellTri[9] = {-29.9375,
                            29.3125,
                            -27.3125,
                            -29.875,
                            29.8125,
                            -27.5,
                            -29.75,
                            27.6875,
                            -27.4375};

  int intersects = vtkIntersectionPolyDataFilter
    ::TriangleTriangleIntersection(&thisCellTri[0],
                                   &thisCellTri[3],
                                   &thisCellTri[6],
                                   &otherCellTri[0],
                                   &otherCellTri[3],
                                   &otherCellTri[6],
                                   coplanar,
                                   isectpt1, isectpt2);

    std::cerr << "First: "
              << thisCellTri[0] << ", "
              << thisCellTri[3] << ", "
              << thisCellTri[6] << std::endl;
    std::cerr << "Second: "
              << otherCellTri[0] << ", "
              << otherCellTri[3] << ", "
              << otherCellTri[6] << std::endl;
  if ( intersects )
    {
    std::cerr << "Triangles with shared vertex should not be reported to intersect" << std::endl;
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
