/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

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
#include "vtkPlaneSource.h"
#include "vtkProbePolyhedron.h"
#include "vtkProperty.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkSmartPointer.h"

int TestMeanValueCoordinatesInterpolation1( int argc, char *argv[] )
{
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = 
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> iren = 
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Create a rectangle
  vtkIdType vertexIds[3] = {0, 1, 2};
  vtkIdType vertexIds1[3] = {2, 3, 0};
  vtkSmartPointer<vtkCellArray> rectCell = vtkSmartPointer<vtkCellArray>::New();
  rectCell->SetNumberOfCells(2);
  rectCell->Initialize();
  rectCell->InsertNextCell(3, vertexIds);
  rectCell->InsertNextCell(3, vertexIds1);
  
  vtkSmartPointer<vtkPoints> rectPoints = vtkSmartPointer<vtkPoints>::New();
  rectPoints->SetNumberOfPoints(4);
  rectPoints->Initialize();
  rectPoints->InsertNextPoint(0.0, -1, -1);
  rectPoints->InsertNextPoint(0.0, -1, 1);
  rectPoints->InsertNextPoint(0.0, 1, 1);
  rectPoints->InsertNextPoint(0.0, 1, -1);

  vtkSmartPointer<vtkPolyData> rectPoly = vtkSmartPointer<vtkPolyData>::New();
  rectPoly->SetPoints(rectPoints);
  rectPoly->SetPolys(rectCell);
  
  vtkSmartPointer<vtkDoubleArray> colorArray = 
    vtkSmartPointer<vtkDoubleArray>::New();
  colorArray->SetNumberOfComponents(1);
  colorArray->SetNumberOfTuples(4);
  colorArray->Initialize();
  
  double r = 0.0;
  double g = 0.5;
  double b = 1.0;
  colorArray->InsertTupleValue(0, &r);
  colorArray->InsertTupleValue(1, &g);
  colorArray->InsertTupleValue(2, &b);
  colorArray->InsertTupleValue(3, &g);
  
  rectPoly->GetPointData()->SetScalars(colorArray);
  
  vtkSmartPointer<vtkPolyDataMapper> rectMapper = 
    vtkSmartPointer<vtkPolyDataMapper>::New();
  rectMapper->SetInput(rectPoly);

  vtkSmartPointer<vtkActor> rectActor = vtkSmartPointer<vtkActor>::New();
  rectActor->SetMapper(rectMapper);

  //Okay now sample the sphere mesh with a plane and see how it interpolates
  vtkSmartPointer<vtkPlaneSource> pSource = 
    vtkSmartPointer<vtkPlaneSource>::New();
  pSource->SetOrigin(0.0,-1.0,-1.0);
  pSource->SetPoint1(0.0, 1.0,-1.0);
  pSource->SetPoint2(0.0,-1.0, 1.0);
  pSource->SetXResolution(50);
  pSource->SetYResolution(50);
  
  vtkSmartPointer<vtkProbePolyhedron> interp = 
    vtkSmartPointer<vtkProbePolyhedron>::New();
  interp->SetInputConnection(pSource->GetOutputPort());
  interp->SetSource(rectPoly);
  
  vtkSmartPointer<vtkPolyDataMapper> interpMapper = 
    vtkSmartPointer<vtkPolyDataMapper>::New();
  interpMapper->SetInputConnection(interp->GetOutputPort());
  
  vtkSmartPointer<vtkActor> interpActor = vtkSmartPointer<vtkActor>::New();
  interpActor->SetMapper(interpMapper);

  vtkSmartPointer<vtkProperty> lightProp = vtkSmartPointer<vtkProperty>::New();
  lightProp->LightingOff();
  rectActor->SetProperty(lightProp);
  interpActor->SetProperty(lightProp);  
  
  //renderer->AddActor(rectActor);
  renderer->AddActor(interpActor);
  renderer->ResetCamera();
  
  renderer->SetBackground(1,1,1);
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}

