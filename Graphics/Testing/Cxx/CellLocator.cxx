/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CellLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkNew.h"
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
  vtkNew<vtkCubeSource> source;
  vtkNew<vtkMaskFields> removearrays;
  removearrays->SetInputConnection(source->GetOutputPort());
  removearrays->CopyAllOff();

  vtkNew<vtkCleanPolyData> clean;
  clean->SetInputConnection(removearrays->GetOutputPort());

  vtkNew<vtkTransform> trans;
  trans->RotateX(6);
  trans->RotateY(9);
  trans->RotateZ(3);

  vtkNew<vtkTransformPolyDataFilter> transformer;
  transformer->SetInputConnection(clean->GetOutputPort());
  transformer->SetTransform(trans.GetPointer());

  vtkNew<vtkTriangleFilter> triangulator;
  triangulator->SetInputConnection(transformer->GetOutputPort());

  vtkNew<vtkLinearSubdivisionFilter> subdivide;
  subdivide->SetInputConnection(triangulator->GetOutputPort());
  subdivide->SetNumberOfSubdivisions(4);
  subdivide->Update();

  vtkNew<vtkPolyData> surface;
  surface->DeepCopy(subdivide->GetOutput());

  // Create the standard locator
  vtkNew<vtkCellLocator> cellLocator;
  cellLocator->SetDataSet(surface.GetPointer());
  cellLocator->BuildLocator();

  // This line (p1,p2) together with the surface mesh
  // generated above reproduces the bug
  double p1[] = {0.897227, 0.0973691, 0.0389687};
  double p2[] = {0.342117, 0.492077, 0.423446};
  vtkNew<vtkIdList> cellIds;
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

int CellLocator( int argc, char *argv[] )
{
  // kuhnan's sample code used to test
  // vtkCellLocator::IntersectWithLine(...9 params...)

  // sphere1: the outer sphere
  vtkSphereSource *sphere1 = vtkSphereSource::New();
  sphere1->SetThetaResolution(100);
  sphere1->SetPhiResolution(100);
  sphere1->SetRadius(1);
  sphere1->Update();

  // sphere2: the inner sphere
  vtkSphereSource *sphere2 = vtkSphereSource::New();
  sphere2->SetThetaResolution(100);
  sphere2->SetPhiResolution(100);
  sphere2->SetRadius(0.8);
  sphere2->Update();

  // the normals obtained from the outer sphere
  vtkDataArray *sphereNormals = sphere1->GetOutput()->GetPointData()->GetNormals();

  // the cell locator
  vtkCellLocator* locator = vtkCellLocator::New();
  locator->SetDataSet(sphere2->GetOutput());
  locator->CacheCellBoundsOn();
  locator->AutomaticOn();
  locator->BuildLocator();

  // init the counter and ray length
  int numIntersected = 0;
  double rayLen = 0.2000001; // = 1 - 0.8 + error tolerance
  int sub_id;
  vtkIdType cell_id;
  double param_t, intersect[3], paraCoord[3];
  double sourcePnt[3], destinPnt[3], normalVec[3];
  vtkGenericCell *cell = vtkGenericCell::New();

  // this loop traverses each point on the outer sphere (sphere1)
  // and  looks for an intersection on the inner sphere (sphere2)
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
    numIntersected ++;
    }

  if ( numIntersected != 9802 )
    {
    int numMissed = 9802 - numIntersected;
    cerr << "ERROR: " << numMissed << " ray-sphere intersections missed!!!" << endl;
    cerr << "If on a non-WinTel32 platform, try rayLen = 0.200001 or 0.20001 for a new test." << endl;
    return 1;
    }
  else
    cout << "Passed: a total of 9802 ray-sphere intersections detected." << endl;

  sphereNormals = NULL;
  cell->Delete();
  sphere1->Delete();
  sphere2->Delete();
  locator->Delete();

  // below: the initial tests

  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(8); sphere->SetPhiResolution(8);
    sphere->SetRadius(1.0);
    sphere->Update();
  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
    sphereMapper->SetInputConnection(sphere->GetOutputPort());
  vtkActor *sphereActor = vtkActor::New();
    sphereActor->SetMapper(sphereMapper);
    
  vtkSphereSource *spot = vtkSphereSource::New();
    spot->SetPhiResolution(6);
    spot->SetThetaResolution(6);
    spot->SetRadius(0.1);

  vtkPolyDataMapper *spotMapper = vtkPolyDataMapper::New();
    spotMapper->SetInputConnection(spot->GetOutputPort());

  // Build a locator 
  vtkCellLocator *cellLocator = vtkCellLocator::New();
  cellLocator->SetDataSet(sphere->GetOutput());
  cellLocator->BuildLocator();

  // Intersect with line
  double p1[] = {2.0, 1.0, 3.0};
  double p2[] = {0.0, 0.0, 0.0};
  double t;
  double ptline[3], pcoords[3];
  int subId;
  cellLocator->IntersectWithLine(p1, p2, 0.001, t, ptline, pcoords, subId);

  vtkActor *intersectLineActor = vtkActor::New();
    intersectLineActor->SetMapper(spotMapper);
    intersectLineActor->SetPosition(ptline[0],ptline[1],ptline[2]);
    intersectLineActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  // Find closest point
  vtkIdType cellId;
  double dist;
  p1[0] = -2.4; p1[1] = -0.9;
  cellLocator->FindClosestPoint(p1, ptline, cellId, subId, dist);
  vtkActor *closestPointActor = vtkActor::New();
    closestPointActor->SetMapper(spotMapper);
    closestPointActor->SetPosition(ptline[0],ptline[1],ptline[2]);
    closestPointActor->GetProperty()->SetColor(0.0, 1.0, 0.0);

  // Find closest point within radius
  float radius = 5.0;
  p1[0] = .2; p1[1] = 1.0; p1[2] = 1.0;
  cellLocator->FindClosestPointWithinRadius(p1, radius, ptline, cellId, subId, dist);
  vtkActor *closestPointActor2 = vtkActor::New();
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

  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();
  spot->Delete();
  spotMapper->Delete();
  intersectLineActor->Delete();
  closestPointActor->Delete();
  closestPointActor2->Delete();
  cellLocator->FreeSearchStructure();
  cellLocator->Delete();

  retVal = retVal & TestFindCellsAlongLine();

  return !retVal;
}
