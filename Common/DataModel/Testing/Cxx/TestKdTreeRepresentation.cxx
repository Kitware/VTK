/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestKdTreeBoxSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkGlyph3D.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkKdTree.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestKdTreeFunctions()
{
  int retVal = 0;

  const int num_points = 10;
  double p[num_points][3] =
  {
    {0.840188, 0.394383, 0.783099},
    {0.79844, 0.911647, 0.197551},
    {0.335223, 0.76823, 0.277775},
    {0.55397, 0.477397, 0.628871},
    {0.364784, 0.513401, 0.95223},
    {0.916195, 0.635712, 0.717297},
    {0.141603, 0.606969, 0.0163006},
    {0.242887, 0.137232, 0.804177},
    {0.156679, 0.400944, 0.12979},
    {0.108809, 0.998925, 0.218257}
  };

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  for (int i = 0; i < num_points; i++)
  {
    points->InsertNextPoint(p[i]);
  }

  vtkSmartPointer<vtkKdTree> kd = vtkSmartPointer<vtkKdTree>::New();
  kd->BuildLocatorFromPoints(points);

  double distance;
  vtkIdType id = kd->FindClosestPoint(0.5, 0.5, 0.5, distance);
  if (id != 3)
  {
    cerr << "FindClosestPoint failed" << endl;
    retVal++;
  }

  double area[6] =
    {
    0.2, 0.8,
    0.2, 0.8,
    0.2, 0.8,
    };
  vtkSmartPointer<vtkIdTypeArray> ids = vtkSmartPointer<vtkIdTypeArray>::New();
  kd->FindPointsInArea(area, ids);
  vtkIdType count = ids->GetNumberOfValues();
  if (count != 2)
  {
    cerr << "FindPointsInArea failed" << endl;
    retVal++;
  }

  double center[3] = {0.0, 0.0, 0.0};
  vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
  kd->FindPointsWithinRadius(10, center, idList);
  vtkIdType n = idList->GetNumberOfIds();
  if (n != 10)
  {
    cerr << "FindPointsWithinRadius failed" << endl;
    retVal++;
  }

  return retVal;
}

int TestKdTreeRepresentation(int argc, char *argv[])
{
  double glyphSize = 0.05;
  const vtkIdType num_points = 10;
  // random points generated on Linux (rand does not work the same on different
  // platforms)
  double p[num_points][3] =
  {
    {0.840188, 0.394383, 0.783099},
    {0.79844, 0.911647, 0.197551},
    {0.335223, 0.76823, 0.277775},
    {0.55397, 0.477397, 0.628871},
    {0.364784, 0.513401, 0.95223},
    {0.916195, 0.635712, 0.717297},
    {0.141603, 0.606969, 0.0163006},
    {0.242887, 0.137232, 0.804177},
    {0.156679, 0.400944, 0.12979},
    {0.108809, 0.998925, 0.218257}
  };

  // generate random points
  VTK_CREATE(vtkPolyData, pointData);
  VTK_CREATE(vtkPoints, points);
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints( num_points );
  pointData->Allocate(num_points);
  for (vtkIdType i = 0; i < num_points; ++i)
  {
    points->SetPoint( i, p[i] );
    pointData->InsertNextCell(VTK_VERTEX,1, &i);
  }
  pointData->SetPoints(points);

  // create a kdtree
  VTK_CREATE(vtkKdTree, kdTree);
  kdTree->SetMinCells(1);
  kdTree->BuildLocatorFromPoints(points);

  // generate a kdtree representation
  VTK_CREATE(vtkPolyData, kdTreeRepr);
  kdTree->GenerateRepresentation(/*kdTree->GetLevel()*/2, kdTreeRepr);
  VTK_CREATE(vtkPolyDataMapper, kdTreeReprMapper);
  kdTreeReprMapper->SetInputData(kdTreeRepr);

  VTK_CREATE(vtkActor, kdTreeReprActor);
  kdTreeReprActor->SetMapper(kdTreeReprMapper);
  kdTreeReprActor->GetProperty()->SetColor(1.0, 1.0, 1.0);
  kdTreeReprActor->GetProperty()->SetRepresentationToWireframe();
  kdTreeReprActor->GetProperty()->SetLineWidth(4);
  kdTreeReprActor->GetProperty()->LightingOff();

  //
  // Create vertex glyphs
  //
  VTK_CREATE(vtkSphereSource, sphere);
  sphere->SetRadius(glyphSize);

  VTK_CREATE(vtkGlyph3D, glyph);
  glyph->SetInputData(0, pointData);
  glyph->SetInputConnection(1, sphere->GetOutputPort());

  VTK_CREATE(vtkPolyDataMapper, glyphMapper);
  glyphMapper->SetInputConnection(glyph->GetOutputPort());

  VTK_CREATE(vtkActor, glyphActor);
  glyphActor->SetMapper(glyphMapper);

  //
  // Set up render window
  //

  VTK_CREATE (vtkCamera, camera);
  vtkSmartPointer<vtkCamera>::New();
  camera->SetPosition(-10, 10, 20);
  camera->SetFocalPoint(0, 0, 0);

  VTK_CREATE(vtkRenderer, ren);
  ren->AddActor(glyphActor);
  ren->AddActor(kdTreeReprActor);
  ren->SetActiveCamera(camera);
  ren->ResetCamera();

  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);
  iren->Initialize();

  VTK_CREATE(vtkInteractorStyleRubberBandPick, interact);
  iren->SetInteractorStyle(interact);

  int retVal = vtkRegressionTestImage (win);
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  retVal = !retVal;
  retVal += TestKdTreeFunctions();
  return retVal;
}
