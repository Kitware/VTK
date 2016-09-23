/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"
#include "vtkDataArray.h"
#include "vtkGenericCell.h"
#include "vtkPointData.h"

#include "vtkActor.h"
#include "vtkCellLocator.h"
#include "vtkCleanPolyData.h"
#include "vtkCubeSource.h"
#include "vtkIdList.h"
#include "vtkLinearSubdivisionFilter.h"
#include "vtkMaskFields.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTriangleFilter.h"

#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

// This test reproduces the cell locator bug in FindCellsAlongLine
int TestFindCellsAlongLine()
{ // returns 1 for success and 0 for failure
  // Generate a surface mesh
  vtkSmartPointer<vtkCubeSource> source =
    vtkSmartPointer<vtkCubeSource>::New();
  vtkSmartPointer<vtkMaskFields> removearrays =
    vtkSmartPointer<vtkMaskFields>::New();
  removearrays->SetInputConnection(source->GetOutputPort());
  removearrays->CopyAllOff();

  vtkSmartPointer<vtkCleanPolyData> clean =
    vtkSmartPointer<vtkCleanPolyData>::New();
  clean->SetInputConnection(removearrays->GetOutputPort());

  vtkSmartPointer<vtkTransform> trans =
    vtkSmartPointer<vtkTransform>::New();
  trans->RotateX(6);
  trans->RotateY(9);
  trans->RotateZ(3);

  vtkSmartPointer<vtkTransformPolyDataFilter> transformer =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  transformer->SetInputConnection(clean->GetOutputPort());
  transformer->SetTransform(trans.GetPointer());

  vtkSmartPointer<vtkTriangleFilter> triangulator =
    vtkSmartPointer<vtkTriangleFilter>::New();
  triangulator->SetInputConnection(transformer->GetOutputPort());

  vtkSmartPointer<vtkLinearSubdivisionFilter> subdivide =
    vtkSmartPointer<vtkLinearSubdivisionFilter>::New();
  subdivide->SetInputConnection(triangulator->GetOutputPort());
  subdivide->SetNumberOfSubdivisions(4);
  subdivide->Update();

  vtkSmartPointer<vtkPolyData> surface =
    vtkSmartPointer<vtkPolyData>::New();
  surface->DeepCopy(subdivide->GetOutput());

  // Create the standard locator
  vtkSmartPointer<vtkCellLocator> cellLocator =
    vtkSmartPointer<vtkCellLocator>::New();
  cellLocator->SetDataSet(surface.GetPointer());
  cellLocator->BuildLocator();

  // This line (p1,p2) together with the surface mesh
  // generated above reproduces the bug
  double p1[] = {0.897227, 0.0973691, 0.0389687};
  double p2[] = {0.342117, 0.492077, 0.423446};
  vtkSmartPointer<vtkIdList> cellIds =
    vtkSmartPointer<vtkIdList>::New();
  cellLocator->FindCellsAlongLine(p1, p2, 0.0, cellIds.GetPointer());

  if(cellIds->GetNumberOfIds() != 4)
  {
    vtkGenericWarningMacro("Wrong amount of intersected Ids " << cellIds->GetNumberOfIds());
    return 0;
  }

  // these ids are the ones that should be in the list.
  // if we uniquely add them the list size should still be 4.
  cellIds->InsertUniqueId(657);
  cellIds->InsertUniqueId(856);
  cellIds->InsertUniqueId(1885);
  cellIds->InsertUniqueId(1887);

  if(cellIds->GetNumberOfIds() != 4)
  {
    vtkGenericWarningMacro("Wrong cell Ids in the list " << cellIds->GetNumberOfIds());
    return 0;
  }

  return 1;
}

int TestCellLocator( int argc, char *argv[] )
{
  // kuhnan's sample code used to test
  // vtkCellLocator::IntersectWithLine(...9 params...)

  // sphere1: the outer sphere
  vtkSmartPointer<vtkSphereSource> sphere1 =
    vtkSmartPointer<vtkSphereSource>::New();
  sphere1->SetThetaResolution(100);
  sphere1->SetPhiResolution(100);
  sphere1->SetRadius(1);
  sphere1->Update();

  // sphere2: the inner sphere
  vtkSmartPointer<vtkSphereSource> sphere2 =
    vtkSmartPointer<vtkSphereSource>::New();
  sphere2->SetThetaResolution(100);
  sphere2->SetPhiResolution(100);
  sphere2->SetRadius(0.8);
  sphere2->Update();

  // the normals obtained from the outer sphere
  vtkDataArray *sphereNormals = sphere1->GetOutput()->GetPointData()->GetNormals();

  // the cell locator
  vtkSmartPointer<vtkCellLocator> locator =
    vtkSmartPointer<vtkCellLocator>::New();
  locator->SetDataSet(sphere2->GetOutput());
  locator->CacheCellBoundsOn();
  locator->AutomaticOn();
  locator->BuildLocator();

  // init the counter and ray length
  int numIntersected = 0;
  double rayLen = 0.200001; // = 1 - 0.8 + error tolerance
  int sub_id;
  vtkIdType cell_id;
  double param_t, intersect[3], paraCoord[3];
  double sourcePnt[3], destinPnt[3], normalVec[3];
  vtkSmartPointer<vtkGenericCell> cell =
    vtkSmartPointer<vtkGenericCell>::New();

  // this loop traverses each point on the outer sphere (sphere1)
  // and  looks for an intersection on the inner sphere (sphere2)
  std::cout << "NumberOfPoints: "
            << sphere1->GetOutput()->GetNumberOfPoints() << std::endl;
  for ( int i = 0; i < sphere1->GetOutput()->GetNumberOfPoints(); i ++ )
  {
    sphere1->GetOutput()->GetPoint(i, sourcePnt);
    sphereNormals->GetTuple(i, normalVec);

    // cast a ray in the negative direction toward sphere1
    destinPnt[0] = sourcePnt[0] - rayLen * normalVec[0];
    destinPnt[1] = sourcePnt[1] - rayLen * normalVec[1];
    destinPnt[2] = sourcePnt[2] - rayLen * normalVec[2];

    if ( locator->IntersectWithLine(sourcePnt, destinPnt, 0.0010, param_t,
                                    intersect, paraCoord, sub_id, cell_id, cell) )
    {
      numIntersected ++;
    }
    else
    {
      std::cout << "Missed intersection: "
                << sourcePnt[0] << ", "
                << sourcePnt[1] << ", "
                << sourcePnt[2] << std::endl;
      std::cout << "To: "
                << destinPnt[0] << ", "
                << destinPnt[1] << ", "
                << destinPnt[2] << std::endl;
      std::cout << "Normal: "
                << normalVec[0] << ", "
                << normalVec[1] << ", "
                << normalVec[2] << std::endl;
    }
  }

  if ( numIntersected != sphere1->GetOutput()->GetNumberOfPoints() )
  {
    int numMissed = sphere1->GetOutput()->GetNumberOfPoints() - numIntersected;
    std::cerr << "ERROR: "
              << numMissed << " ray-sphere intersections missed!!!"
              << std::endl;
    std::cerr << "If on a non-WinTel32 platform, try rayLen = 0.200001 or 0.20001 for a new test." << std::endl;
    return 1;
  }
  else
  {
    std::cout << "Passed: a total of "
              << sphere1->GetOutput()->GetNumberOfPoints()
              << " ray-sphere intersections detected." << std::endl;
  }
  sphereNormals = NULL;

  // below: the initial tests

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetThetaResolution(8); sphere->SetPhiResolution(8);
  sphere->SetRadius(1.0);
  sphere->Update();

  vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  vtkSmartPointer<vtkActor> sphereActor =
    vtkSmartPointer<vtkActor>::New();
  sphereActor->SetMapper(sphereMapper);

  vtkSmartPointer<vtkSphereSource> spot =
    vtkSmartPointer<vtkSphereSource>::New();
  spot->SetPhiResolution(6);
  spot->SetThetaResolution(6);
  spot->SetRadius(0.1);

  vtkSmartPointer<vtkPolyDataMapper> spotMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  spotMapper->SetInputConnection(spot->GetOutputPort());

  // Build a locator
  vtkSmartPointer<vtkCellLocator> cellLocator =
    vtkSmartPointer<vtkCellLocator>::New();
  cellLocator->SetDataSet(sphere->GetOutput());
  cellLocator->BuildLocator();

  // Intersect with line
  double p1[] = {2.0, 1.0, 3.0};
  double p2[] = {0.0, 0.0, 0.0};
  double t;
  double ptline[3], pcoords[3];
  int subId;
  cellLocator->IntersectWithLine(p1, p2, 0.001, t, ptline, pcoords, subId);

  vtkSmartPointer<vtkActor> intersectLineActor =
    vtkSmartPointer<vtkActor>::New();
  intersectLineActor->SetMapper(spotMapper);
  intersectLineActor->SetPosition(ptline[0],ptline[1],ptline[2]);
  intersectLineActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  // Find closest point
  vtkIdType cellId;
  double dist;
  p1[0] = -2.4; p1[1] = -0.9;
  cellLocator->FindClosestPoint(p1, ptline, cellId, subId, dist);
  vtkSmartPointer<vtkActor> closestPointActor =
    vtkSmartPointer<vtkActor>::New();
  closestPointActor->SetMapper(spotMapper);
  closestPointActor->SetPosition(ptline[0],ptline[1],ptline[2]);
  closestPointActor->GetProperty()->SetColor(0.0, 1.0, 0.0);

  // Find closest point within radius
  float radius = 5.0;
  p1[0] = .2; p1[1] = 1.0; p1[2] = 1.0;
  cellLocator->FindClosestPointWithinRadius(p1, radius, ptline, cellId, subId, dist);
  vtkSmartPointer<vtkActor> closestPointActor2 =
    vtkSmartPointer<vtkActor>::New();
  closestPointActor2->SetMapper(spotMapper);
  closestPointActor2->SetPosition(ptline[0],ptline[1],ptline[2]);
  closestPointActor2->GetProperty()->SetColor(0.0, 1.0, 0.0);

  renderer->AddActor(sphereActor);
  renderer->AddActor(intersectLineActor);
  renderer->AddActor(closestPointActor);
  renderer->AddActor(closestPointActor2);
  renderer->SetBackground(1,1,1);
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  retVal = retVal & TestFindCellsAlongLine();

  return !retVal;
}
