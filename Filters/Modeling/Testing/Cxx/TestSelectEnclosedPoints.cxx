/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSelectEnclosedPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates the use of vtkGenerateSurfaceIntersectionLines
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

// If WRITE_RESULT is defined, the result of the surface filter is saved.
//#define WRITE_RESULT

#include "vtkActor.h"
#include "vtkDataArray.h"
#include "vtkGlyph3D.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRandomPool.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelectEnclosedPoints.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkThresholdPoints.h"
#include "vtkTimerLog.h"

int TestSelectEnclosedPoints(int argc, char* argv[])
{
  // Standard rendering classes
  vtkRenderer* renderer = vtkRenderer::New();
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Create a containing surface
  vtkSphereSource* ss = vtkSphereSource::New();
  ss->SetPhiResolution(25);
  ss->SetThetaResolution(38);
  ss->SetCenter(4.5, 5.5, 5.0);
  ss->SetRadius(2.5);
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(ss->GetOutputPort());
  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToWireframe();

  // Generate some random points
  vtkPoints* points = vtkPoints::New();
  points->SetNumberOfPoints(500);

  vtkDataArray* da = points->GetData();
  vtkRandomPool* pool = vtkRandomPool::New();
  pool->PopulateDataArray(da, 0, 2.25, 7);
  pool->PopulateDataArray(da, 1, 1, 10);
  pool->PopulateDataArray(da, 2, 0.5, 10.5);

  vtkPolyData* profile = vtkPolyData::New();
  profile->SetPoints(points);

  vtkSelectEnclosedPoints* select = vtkSelectEnclosedPoints::New();
  select->SetInputData(profile);
  select->SetSurfaceConnection(ss->GetOutputPort());
  //  select->InsideOutOn();

  // Time execution
  vtkTimerLog* timer = vtkTimerLog::New();
  timer->StartTimer();
  select->Update();
  timer->StopTimer();
  double time = timer->GetElapsedTime();
  cout << "Time to extract points: " << time << "\n";

  // Now extract points
  vtkThresholdPoints* thresh = vtkThresholdPoints::New();
  thresh->SetInputConnection(select->GetOutputPort());
  thresh->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "SelectedPoints");
  thresh->ThresholdByUpper(0.9);

  vtkSphereSource* glyph = vtkSphereSource::New();
  vtkGlyph3D* glypher = vtkGlyph3D::New();
  glypher->SetInputConnection(thresh->GetOutputPort());
  glypher->SetSourceConnection(glyph->GetOutputPort());
  glypher->SetScaleModeToDataScalingOff();
  glypher->SetScaleFactor(0.25);

  vtkPolyDataMapper* pointsMapper = vtkPolyDataMapper::New();
  pointsMapper->SetInputConnection(glypher->GetOutputPort());
  pointsMapper->ScalarVisibilityOff();

  vtkActor* pointsActor = vtkActor::New();
  pointsActor->SetMapper(pointsMapper);
  pointsActor->GetProperty()->SetColor(0, 0, 1);

  // Add actors
  //  renderer->AddActor(actor);
  renderer->AddActor(pointsActor);

  // Standard testing code.
  renWin->SetSize(300, 300);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Cleanup
  renderer->Delete();
  renWin->Delete();
  iren->Delete();

  ss->Delete();
  mapper->Delete();
  actor->Delete();

  pool->Delete();
  timer->Delete();

  points->Delete();
  profile->Delete();
  thresh->Delete();
  select->Delete();
  glyph->Delete();
  glypher->Delete();
  pointsMapper->Delete();
  pointsActor->Delete();

  return !retVal;
}
