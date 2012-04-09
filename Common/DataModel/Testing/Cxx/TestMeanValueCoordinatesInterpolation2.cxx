/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMeanValueCoordinatesInterpolation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkDebugLeaks.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderer.h"
#include "vtkPlane.h"
#include "vtkPlaneSource.h"
#include "vtkProbeFilter.h"
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkSmartPointer.h"

// Test MVC interpolation of polygon cell
int TestMeanValueCoordinatesInterpolation2( int argc, char *argv[] )
{
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderer> renderer1 =
    vtkSmartPointer<vtkRenderer>::New();
  renderer->SetViewport(0, 0, 0.5, 1);

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  renWin->AddRenderer(renderer1);
  renderer1->SetViewport(0.5, 0, 1, 1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  //
  // Case 0: convex pentagon
  //
  // create a regular pentagon
  double pentagon[5][3];
  for (int i = 0; i < 5; i++)
    {
    pentagon[i][0] = sin(vtkMath::RadiansFromDegrees(72.0*i));
    pentagon[i][1] = cos(vtkMath::RadiansFromDegrees(72.0*i));
    pentagon[i][2] = 0.0;
    }

  vtkSmartPointer<vtkCellArray> pentagonCell = vtkSmartPointer<vtkCellArray>::New();
  pentagonCell->InsertNextCell(5);
  for (vtkIdType i = 0; i < 5; i++)
    {
    pentagonCell->InsertCellPoint(i);
    }

  vtkSmartPointer<vtkPoints> pentagonPoints = vtkSmartPointer<vtkPoints>::New();
  pentagonPoints->Initialize();
  for (int i = 0; i < 5; i++)
    {
    pentagonPoints->InsertNextPoint(pentagon[i]);
    }

  vtkSmartPointer<vtkDoubleArray> pointDataArray =
    vtkSmartPointer<vtkDoubleArray>::New();
  pointDataArray->Initialize();
  for (int i = 0; i < 5; i++)
    {
    pointDataArray->InsertNextValue((pentagon[i][0]+1.0)/2.0);
    }

  vtkSmartPointer<vtkPolyData> polydata =
    vtkSmartPointer<vtkPolyData>::New();
  polydata->SetPoints(pentagonPoints);
  polydata->SetPolys(pentagonCell);
  polydata->GetPointData()->SetScalars(pointDataArray);

  vtkPolygon *polygon = static_cast<vtkPolygon*>(polydata->GetCell(0));
  polygon->SetUseMVCInterpolation(1);


  //Okay now sample on a plane and see how it interpolates
  vtkSmartPointer<vtkPlaneSource> pSource =
    vtkSmartPointer<vtkPlaneSource>::New();
  pSource->SetOrigin(-1.0,-1.0,0);
  pSource->SetPoint1(1.0,-1.0,0);
  pSource->SetPoint2(-1.0, 1.0,0);
  pSource->SetXResolution(100);
  pSource->SetYResolution(100);

  // mvc interpolation
  vtkSmartPointer<vtkProbeFilter> interp =
    vtkSmartPointer<vtkProbeFilter>::New();
  interp->SetInputConnection(pSource->GetOutputPort());
  interp->SetSourceData(polydata);

  vtkSmartPointer<vtkPolyDataMapper> interpMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  interpMapper->SetInputConnection(interp->GetOutputPort());

  vtkSmartPointer<vtkActor> interpActor = vtkSmartPointer<vtkActor>::New();
  interpActor->SetMapper(interpMapper);

  //
  // Case 1: convex polygon meshes
  //
  pentagon[0][0] = 0.0;
  pentagon[0][1] = 0.0;
  pentagon[0][2] = 0.0;

  vtkSmartPointer<vtkPoints> pentagonPoints1 = vtkSmartPointer<vtkPoints>::New();
  pentagonPoints1->Initialize();
  for (int i = 0; i < 5; i++)
    {
    pentagonPoints1->InsertNextPoint(pentagon[i]);
    }

  vtkSmartPointer<vtkCellArray> pentagonCell1 = vtkSmartPointer<vtkCellArray>::New();
  pentagonCell1->InsertNextCell(5);
  for (vtkIdType i = 0; i < 5; i++)
    {
    pentagonCell1->InsertCellPoint(i);
    }

  vtkSmartPointer<vtkDoubleArray> pointDataArray1 =
    vtkSmartPointer<vtkDoubleArray>::New();
  pointDataArray1->Initialize();
  for (int i = 0; i < 5; i++)
    {
    pointDataArray1->InsertNextValue((pentagon[i][0]+1.0)/2.0);
    }

  vtkSmartPointer<vtkPolyData> polydata1 =
    vtkSmartPointer<vtkPolyData>::New();
  polydata1->SetPoints(pentagonPoints1);
  polydata1->SetPolys(pentagonCell1);
  polydata1->GetPointData()->SetScalars(pointDataArray1);

  vtkPolygon *polygon1 = static_cast<vtkPolygon*>(polydata1->GetCell(0));
  polygon1->SetUseMVCInterpolation(1);

  //Okay now sample on a plane and see how it interpolates
  vtkSmartPointer<vtkPlaneSource> pSource1 =
    vtkSmartPointer<vtkPlaneSource>::New();
  pSource1->SetOrigin(-1.0,-1.0,0);
  pSource1->SetPoint1(1.0,-1.0,0);
  pSource1->SetPoint2(-1.0, 1.0,0);
  pSource1->SetXResolution(100);
  pSource1->SetYResolution(100);

  // interpolation 1: use the more general but slower MVC algorithm.
  vtkSmartPointer<vtkProbeFilter> interp1 =
    vtkSmartPointer<vtkProbeFilter>::New();
  interp1->SetInputConnection(pSource1->GetOutputPort());
  interp1->SetSourceData(polydata1);

  vtkSmartPointer<vtkPolyDataMapper> interpMapper1 =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  interpMapper1->SetInputConnection(interp1->GetOutputPort());

  vtkSmartPointer<vtkActor> interpActor1 = vtkSmartPointer<vtkActor>::New();
  interpActor1->SetMapper(interpMapper1);

  //
  // add actors to renderer
  //
  vtkSmartPointer<vtkProperty> lightProperty =
    vtkSmartPointer<vtkProperty>::New();
  lightProperty->LightingOff();
  interpActor->SetProperty(lightProperty);
  interpActor1->SetProperty(lightProperty);

  renderer->AddActor(interpActor);
  renderer->ResetCamera();
  renderer->SetBackground(1,1,1);

  renderer1->AddActor(interpActor1);
  renderer1->ResetCamera();
  renderer1->SetBackground(1,1,1);

  renWin->SetSize(600,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}

