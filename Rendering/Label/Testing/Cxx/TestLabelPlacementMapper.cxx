/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLabelPlacementMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of vtkLabelPlacementMapper
// .SECTION Description
// this program tests vtkLabelPlacementMapper which uses a sophisticated algorithm to
// prune labels/icons preventing them from overlapping.

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkLabelHierarchy.h"
#include "vtkLabelPlacementMapper.h"
#include "vtkLabeledDataMapper.h"
#include "vtkPointData.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkPointSource.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty.h"
#include "vtkRectilinearGrid.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLPolyDataWriter.h"

#include <vtkRegressionTestImage.h>
#include <vtkTestUtilities.h>

int TestLabelPlacementMapper(int argc, char* argv[])
{
  // use non-unit aspect ratio to capture more potential errors
  int windowSize[2] = { 200, 600 };
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkInteractorStyleSwitch::SafeDownCast(iren->GetInteractorStyle())
    ->SetCurrentStyleToTrackballCamera();

  renWin->SetSize(200, 600);
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.0, 0.0, 0.0);
  iren->SetRenderWindow(renWin);

  vtkNew<vtkTextProperty> tprop;
  tprop->SetFontSize(12);
  tprop->SetFontFamily(vtkTextProperty::GetFontFamilyFromString("Arial"));
  tprop->SetColor(0.0, 0.8, 0.2);

  // Test display if anchor is defined in
  // World coordinate system
  {
    int maxLevels = 5;
    int targetLabels = 32;
    double labelRatio = 0.05;
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/uniform-001371-5x5x5.vtp");
    // int iteratorType = vtkLabelHierarchy::FULL_SORT;
    int iteratorType = vtkLabelHierarchy::QUEUE;
    // int iteratorType = vtkLabelHierarchy::DEPTH_FIRST;
    double center[3] = { 12.0, 8.0, 30.0 };

    vtkNew<vtkSphereSource> sphere;
    sphere->SetRadius(5.0);
    // view will be centered around this centerpoint, thereby shifting normalized view coordinate
    // system from world coordinate system (to test if label display works with acnhors defined in
    // arbitrary coordinate systems).
    sphere->SetCenter(center);
    vtkNew<vtkPolyDataMapper> sphereMapper;
    sphereMapper->SetInputConnection(sphere->GetOutputPort());
    vtkNew<vtkActor> sphereActor;
    sphereActor->SetMapper(sphereMapper);
    renderer->AddActor(sphereActor);

    vtkNew<vtkXMLPolyDataReader> xmlPolyDataReader;
    xmlPolyDataReader->SetFileName(fname);
    delete[] fname;

    vtkNew<vtkTransformPolyDataFilter> transformToCenter;
    transformToCenter->SetInputConnection(xmlPolyDataReader->GetOutputPort());
    vtkNew<vtkTransform> transformToCenterTransform;
    transformToCenterTransform->Translate(center);
    transformToCenter->SetTransform(transformToCenterTransform);

    vtkNew<vtkPointSetToLabelHierarchy> pointSetToLabelHierarchy;
    pointSetToLabelHierarchy->SetTextProperty(tprop);
    pointSetToLabelHierarchy->AddInputConnection(transformToCenter->GetOutputPort());
    pointSetToLabelHierarchy->SetPriorityArrayName("Priority");
    pointSetToLabelHierarchy->SetLabelArrayName("PlaceNames");
    pointSetToLabelHierarchy->SetMaximumDepth(maxLevels);
    pointSetToLabelHierarchy->SetTargetLabelCount(targetLabels);

    vtkNew<vtkLabelPlacementMapper> labelPlacer;
    labelPlacer->SetInputConnection(pointSetToLabelHierarchy->GetOutputPort());
    labelPlacer->SetIteratorType(iteratorType);
    labelPlacer->SetMaximumLabelFraction(labelRatio);
    labelPlacer->UseDepthBufferOn();

    vtkNew<vtkActor2D> textActor;
    textActor->SetMapper(labelPlacer);
    renderer->AddActor(textActor);
  }

  // Test display if anchor is defined in
  // NormalizedViewport coordinate system
  {
    vtkNew<vtkPolyData> labeledPoints;
    vtkNew<vtkPoints> points;
    points->InsertNextPoint(0.05, 0.25, 0);
    points->InsertNextPoint(0.75, 0.75, 0);
    points->InsertNextPoint(0.50, 0.05, 0);
    points->InsertNextPoint(0.50, 0.95, 0);
    labeledPoints->SetPoints(points);
    vtkNew<vtkStringArray> labels;
    labels->SetName("labels");
    labels->InsertNextValue("NV-left");
    labels->InsertNextValue("NV-right");
    labels->InsertNextValue("NV-bottom");
    labels->InsertNextValue("NV-top");
    vtkNew<vtkStringArray> labelsPriority;
    labelsPriority->SetName("priority");
    labelsPriority->InsertNextValue("1");
    labelsPriority->InsertNextValue("1");
    labelsPriority->InsertNextValue("1");
    labelsPriority->InsertNextValue("1");
    labeledPoints->GetPointData()->AddArray(labels);
    labeledPoints->GetPointData()->AddArray(labelsPriority);
    vtkNew<vtkPointSetToLabelHierarchy> pointSetToLabelHierarchy;
    pointSetToLabelHierarchy->SetTextProperty(tprop);
    pointSetToLabelHierarchy->AddInputData(labeledPoints);
    pointSetToLabelHierarchy->SetPriorityArrayName("priority");
    pointSetToLabelHierarchy->SetLabelArrayName("labels");
    vtkNew<vtkLabelPlacementMapper> labelPlacer;
    labelPlacer->SetInputConnection(pointSetToLabelHierarchy->GetOutputPort());
    labelPlacer->PlaceAllLabelsOn();
    labelPlacer->GetAnchorTransform()->SetCoordinateSystemToNormalizedViewport();
    labelPlacer->UseDepthBufferOff();
    vtkNew<vtkActor2D> textActor;
    textActor->SetMapper(labelPlacer);
    renderer->AddActor(textActor);
  }

  // Test display if anchor is defined in
  // Display coordinate system
  {
    vtkNew<vtkPolyData> labeledPoints;
    vtkNew<vtkPoints> points;
    points->InsertNextPoint(windowSize[0] * 0.01, windowSize[1] * 0.01, 0);
    points->InsertNextPoint(windowSize[0] * 0.90, windowSize[1] * 0.01, 0);
    points->InsertNextPoint(windowSize[0] * 0.01, windowSize[1] * 0.97, 0);
    points->InsertNextPoint(windowSize[0] * 0.90, windowSize[1] * 0.97, 0);
    labeledPoints->SetPoints(points);
    vtkNew<vtkStringArray> labels;
    labels->SetName("labels");
    labels->InsertNextValue("D-bottom-left");
    labels->InsertNextValue("D-bottom-right");
    labels->InsertNextValue("D-top-left");
    labels->InsertNextValue("D-top-right");
    vtkNew<vtkStringArray> labelsPriority;
    labelsPriority->SetName("priority");
    labelsPriority->InsertNextValue("1");
    labelsPriority->InsertNextValue("1");
    labelsPriority->InsertNextValue("1");
    labelsPriority->InsertNextValue("1");
    labeledPoints->GetPointData()->AddArray(labels);
    labeledPoints->GetPointData()->AddArray(labelsPriority);
    vtkNew<vtkPointSetToLabelHierarchy> pointSetToLabelHierarchy;
    pointSetToLabelHierarchy->SetTextProperty(tprop);
    pointSetToLabelHierarchy->AddInputData(labeledPoints);
    pointSetToLabelHierarchy->SetPriorityArrayName("priority");
    pointSetToLabelHierarchy->SetLabelArrayName("labels");
    vtkNew<vtkLabelPlacementMapper> labelPlacer;
    labelPlacer->SetInputConnection(pointSetToLabelHierarchy->GetOutputPort());
    labelPlacer->PlaceAllLabelsOn();
    labelPlacer->GetAnchorTransform()->SetCoordinateSystemToDisplay();
    labelPlacer->UseDepthBufferOff();
    vtkNew<vtkActor2D> textActor;
    textActor->SetMapper(labelPlacer);
    renderer->AddActor(textActor);
  }

  ///

  renWin->Render();
  // renderer->GetActiveCamera()->ParallelProjectionOn();
  renderer->ResetCamera();
  renderer->ResetCamera();
  renderer->ResetCamera();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
