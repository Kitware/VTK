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
  double thisCellTri[9] = {-30.121610641479492,
                           29.366317749023438,
                           -27.164772033691406,
                           -29.923696517944336,
                           29.366317749023438,
                           -27.334911346435547,
                           -30.055425643920898,
                           28.556877136230469,
                           -27.227336883544922};
  double otherCellTri[9] = {-29.923696517944336,
                            29.366317749023438,
                            -27.334911346435547,
                            -29.858596801757812,
                            29.366317749023438,
                            -27.390081405639648,
                            -29.813289642333984,
                            27.682807922363281,
                            -27.436887741088867};

  int intersects = vtkIntersectionPolyDataFilter
    ::TriangleTriangleIntersection(&thisCellTri[0],
                                   &thisCellTri[3],
                                   &thisCellTri[6],
                                   &otherCellTri[0],
                                   &otherCellTri[3],
                                   &otherCellTri[6],
                                   coplanar,
                                   isectpt1, isectpt2);

  if ( intersects )
    {
    cerr << "Triangles with shared vertex should not be reported to intersect" << endl;
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
