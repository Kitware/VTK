/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestClipLabels.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of clipping with vtkLabeledDataMapper
// .SECTION Description
// this program tests that clipping planes affect labels

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkCellCenters.h"
#include "vtkIdFilter.h"
#include "vtkLabeledDataMapper.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelectVisiblePoints.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"

int TestClipLabels(int argc, char* argv[])
{
  // Selecting point/cells within the entire window
  int xmin = 0;
  int ymin = 0;
  int xmax = 400;
  int ymax = 400;

  // Create a sphere and its associated mapper and actor.
  vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
  vtkSmartPointer<vtkPolyDataMapper> sphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  vtkSmartPointer<vtkActor> sphereActor = vtkSmartPointer<vtkActor>::New();
  sphereActor->SetMapper(sphereMapper);

  // Generate data arrays containing point and cell ids
  vtkSmartPointer<vtkIdFilter> ids = vtkSmartPointer<vtkIdFilter>::New();
  ids->SetInputConnection(sphere->GetOutputPort());
  ids->PointIdsOn();
  ids->CellIdsOn();
  ids->FieldDataOn();

  // Create the renderer here because vtkSelectVisiblePoints needs it.
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();

  // Create labels for points
  vtkSmartPointer<vtkSelectVisiblePoints> visPts = vtkSmartPointer<vtkSelectVisiblePoints>::New();
  visPts->SetInputConnection(ids->GetOutputPort());
  visPts->SetRenderer(ren1);
  visPts->SelectionWindowOn();
  visPts->SetSelection(xmin, xmax, ymin, ymax);

  // Create the mapper to display the point ids.  Specify the
  // format to use for the labels.  Also create the associated actor.
  vtkSmartPointer<vtkLabeledDataMapper> pointMapper = vtkSmartPointer<vtkLabeledDataMapper>::New();
  pointMapper->SetInputConnection(visPts->GetOutputPort());
  pointMapper->SetLabelModeToLabelFieldData();

  vtkSmartPointer<vtkActor2D> pointLabels = vtkSmartPointer<vtkActor2D>::New();
  pointLabels->SetMapper(pointMapper);

  // Create labels for cells
  vtkSmartPointer<vtkCellCenters> cc = vtkSmartPointer<vtkCellCenters>::New();
  cc->SetInputConnection(ids->GetOutputPort());

  vtkSmartPointer<vtkSelectVisiblePoints> visCells = vtkSmartPointer<vtkSelectVisiblePoints>::New();
  visCells->SetInputConnection(cc->GetOutputPort());
  visCells->SetRenderer(ren1);
  visCells->SelectionWindowOn();
  visCells->SetSelection(xmin, xmax, ymin, ymax);

  // Create the mapper to display the cell ids.  Specify the
  // format to use for the labels.  Also create the associated actor.
  vtkSmartPointer<vtkLabeledDataMapper> cellMapper = vtkSmartPointer<vtkLabeledDataMapper>::New();
  cellMapper->SetInputConnection(visCells->GetOutputPort());
  cellMapper->SetLabelModeToLabelFieldData();
  cellMapper->GetLabelTextProperty()->SetColor(0, 1, 0);

  vtkSmartPointer<vtkActor2D> cellLabels = vtkSmartPointer<vtkActor2D>::New();
  cellLabels->SetMapper(cellMapper);

  // Create the RenderWindow and RenderWindowInteractor
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);
  renWin->SetSize(xmax, ymax);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Add the actors to the renderer; set the background and size; render
  ren1->AddActor(sphereActor);

  ren1->SetBackground(1, 1, 1);
  renWin->SetSize(xmax, ymax);
  renWin->Render();

  vtkNew<vtkPlane> clipPlane1;
  clipPlane1->SetOrigin(-.1, 0.0, 0.0);
  clipPlane1->SetNormal(1, 0, 0.0);
  vtkNew<vtkPlane> clipPlane2;
  clipPlane2->SetOrigin(0.1, 0.0, 0.0);
  clipPlane2->SetNormal(-1, 0, 0.0);

  vtkNew<vtkPlaneCollection> clipPlaneCollection;
  clipPlaneCollection->AddItem(clipPlane1);
  clipPlaneCollection->AddItem(clipPlane2);
  sphereMapper->SetClippingPlanes(clipPlaneCollection);
  pointMapper->SetClippingPlanes(clipPlaneCollection);
  cellMapper->SetClippingPlanes(clipPlaneCollection);
  ren1->AddActor2D(pointLabels);
  ren1->AddActor2D(cellLabels);

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
