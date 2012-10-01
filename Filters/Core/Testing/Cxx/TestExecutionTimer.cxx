/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExecutionTimer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


// This test uses the guts of TestDelaunay2D.  I just attach a
// vtkExecutionTimer to vtkDelaunay2D so that I can watch
// something non-trivial.

#include "vtkExecutionTimer.h"

#include "vtkCellArray.h"
#include "vtkDelaunay2D.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

#include <cstdlib>

int TestExecutionTimer( int vtkNotUsed(argc), char* vtkNotUsed(argv)[] )
{
  vtkPoints *newPts = vtkPoints::New();
  newPts->InsertNextPoint(  1.5026018771810041,  1.5026019428618222, 0.0 );
  newPts->InsertNextPoint( -1.5026020085426373,  1.5026018115001829, 0.0 );
  newPts->InsertNextPoint( -1.5026018353814194, -1.5026019846614038, 0.0 );
  newPts->InsertNextPoint(  1.5026019189805875, -1.5026019010622396, 0.0 );
  newPts->InsertNextPoint(  5.2149123972752491,  5.2149126252263240, 0.0 );
  newPts->InsertNextPoint( -5.2149128531773883,  5.2149121693241645, 0.0 );
  newPts->InsertNextPoint( -5.2149122522061022, -5.2149127702954603, 0.0 );
  newPts->InsertNextPoint(  5.2149125423443916, -5.2149124801571842, 0.0 );
  newPts->InsertNextPoint(  8.9272229173694946,  8.9272233075908254, 0.0 );
  newPts->InsertNextPoint( -8.9272236978121402,  8.9272225271481460, 0.0 );
  newPts->InsertNextPoint( -8.9272226690307868, -8.9272235559295172, 0.0 );
  newPts->InsertNextPoint(  8.9272231657081953, -8.9272230592521282, 0.0 );
  newPts->InsertNextPoint(  12.639533437463740,  12.639533989955329, 0.0 );
  newPts->InsertNextPoint( -12.639534542446890,  12.639532884972127, 0.0 );
  newPts->InsertNextPoint( -12.639533085855469, -12.639534341563573, 0.0 );
  newPts->InsertNextPoint(  12.639533789072001, -12.639533638347073, 0.0 );

  vtkPolyData *pointCloud = vtkPolyData::New();
  pointCloud->SetPoints(newPts);
  newPts->Delete();

  vtkDelaunay2D *delaunay2D = vtkDelaunay2D::New();
  delaunay2D->SetInputData( pointCloud );
  pointCloud->Delete();

  vtkSmartPointer<vtkExecutionTimer> timer = vtkSmartPointer<vtkExecutionTimer>::New();
  timer->SetFilter(delaunay2D);

  delaunay2D->Update();

  std::cout << "TestExecutionTimer: Filter under inspection ("
            << timer->GetFilter()->GetClassName() << ") execution time: "
            << std::fixed << std::setprecision(8)
            << timer->GetElapsedCPUTime() << " sec (CPU), "
            << timer->GetElapsedWallClockTime() << " sec (wall clock)\n";

  delaunay2D->Delete();

  // As long as the thing executes without crashing, the test is
  // successful.
  return EXIT_SUCCESS;
}
