/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyLineSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <cstdio>

#include <vtkPolyLineSource.h>
#include <vtkSmartPointer.h>

int TestPolyLineSource(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkPolyLineSource> source = vtkSmartPointer<vtkPolyLineSource>::New();

  // Basic tests
  int expectedNumberOfPoints = 4;
  source->SetNumberOfPoints( 4 );
  int actualNumberOfPoints = source->GetNumberOfPoints();
  if (expectedNumberOfPoints != actualNumberOfPoints)
  {
    std::cerr << "Expected NumberOfPoints setting to be " << expectedNumberOfPoints << ", got "
              << actualNumberOfPoints << std::endl;
    return EXIT_FAILURE;
  }

  int expectedClosed = 1;
  source->SetClosed( expectedClosed );
  int actualClosed = source->GetClosed();
  if (expectedClosed != actualClosed)
  {
    std::cerr << "Expected Closed setting to be " << expectedClosed << ", got "
              << actualClosed << std::endl;
    return EXIT_FAILURE;
  }

  // Test setting individual points
  double pts[4][3] = {
    { 1.0, 2.0, 3.0 },
    { 4.0, 5.0, 6.0 },
    { 7.0, 8.0, 9.0 },
    { 10.0, 11.0, 12.0 }
  };

  for ( int i = 0; i < 4; ++i )
  {
    source->SetPoint( i, pts[i][0], pts[i][1], pts[i][2] );
  }

  // Test getting the points
  vtkPoints* testPoints = source->GetPoints();
  if ( testPoints->GetNumberOfPoints() != 4 )
  {
    std::cerr << "Expected 4 points in vtkPoints returned from GetPoints() method, but got "
              << testPoints->GetNumberOfPoints() << " instead." << std::endl;
    return EXIT_FAILURE;
  }

  // Test the point values
  vtkPoints* outputPoints = source->GetPoints();
  for ( int i = 0; i < 4; ++i )
  {
    double pt[3];
    outputPoints->GetPoint(i, pt);

    if ( pt[0] != pts[i][0] || pt[1] != pts[i][1] || pt[2] != pts[i][2] )
    {
      std::cerr << "Point disagreement in point " << i << std::endl;
      std::cerr << "Expected point: " << pts[i][0] << ", " << pts[i][1] << ", " << pts[i][2] << std::endl;
      std::cerr << "Actual point:  " << pt[0] << ", " << pt[1] << ", " << pt[2] << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test setting the points from a vtkPoints object
  double newPts[3][3] = {
    { 13.0, 14.0, 15.0 },
    { 16.0, 17.0, 18.0 },
    { 19.0, 20.0, 21.0 }
  };

  vtkSmartPointer<vtkPoints> newPoints = vtkSmartPointer<vtkPoints>::New();
  newPoints->SetNumberOfPoints( 3 );
  for ( int i = 0; i < 3; ++i )
  {
    newPoints->SetPoint( i, newPts[i] );
  }
  source->SetPoints( newPoints );

  actualNumberOfPoints = source->GetNumberOfPoints();
  if ( source->GetNumberOfPoints() != 3)
  {
    std::cerr << "Expected 3 points, got " << source->GetNumberOfPoints() << std::endl;
    return EXIT_FAILURE;
  }

  outputPoints = source->GetPoints();
  for ( int i = 0; i < 3; ++i )
  {
    double pt[3];
    outputPoints->GetPoint(i, pt);

    if ( pt[0] != newPts[i][0] || pt[1] != newPts[i][1] || pt[2] != newPts[i][2] )
    {
      std::cerr << "Point disagreement in point " << i << std::endl;
      std::cerr << "Expected point: " << newPts[i][0] << ", " << newPts[i][1] << ", " << newPts[i][2] << std::endl;
      std::cerr << "Actual point:  " << pt[0] << ", " << pt[1] << ", " << pt[2] << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
