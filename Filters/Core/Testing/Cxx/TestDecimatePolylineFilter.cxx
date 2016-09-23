/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDecimatePolylineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkDecimatePolylineFilter.h>
#include <vtkMath.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include "vtkRegressionTestImage.h"
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>

int TestDecimatePolylineFilter(int argc, char *argv[])
{
  const unsigned int numberOfPointsInCircle = 100;

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataType(VTK_FLOAT);

  // We will create two polylines: one complete circle, and one circular arc
  // subtending 3/4 of a circle.
  vtkIdType *lineIds = new vtkIdType[(numberOfPointsInCircle*7)/4+1];

  vtkIdType lineIdCounter = 0;

  // First circle:
  for(unsigned int i = 0; i < numberOfPointsInCircle; ++i)
  {
    const double angle = 2.0 * vtkMath::Pi() * static_cast<double>(i)
      / static_cast<double>(numberOfPointsInCircle);
    points->InsertPoint(static_cast<vtkIdType>(i), std::cos(angle),
      std::sin(angle), 0.0);
    lineIds[i] = lineIdCounter++;
  }
  lineIds[numberOfPointsInCircle] = 0;

  // Second circular arc:
  for(unsigned int i = 0; i < (numberOfPointsInCircle*3)/4; ++i)
  {
    const double angle = 3.0 / 2.0 * vtkMath::Pi() * static_cast<double>(i)
      / static_cast<double>((numberOfPointsInCircle*3)/4);
    points->InsertPoint(static_cast<vtkIdType>(i+numberOfPointsInCircle),
                        std::cos(angle), std::sin(angle), 1.0);
    lineIds[numberOfPointsInCircle + 1 + i] = lineIdCounter++;
  }

  // Construct associated cell array, containing both polylines.
  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  // 1st:
  lines->InsertNextCell(numberOfPointsInCircle + 1, lineIds);
  // 2nd:
  lines->InsertNextCell((numberOfPointsInCircle*3)/4,
                        &lineIds[numberOfPointsInCircle+1]);
  delete[] lineIds;

  vtkSmartPointer<vtkPolyData> circles = vtkSmartPointer<vtkPolyData>::New();
  circles->SetPoints(points);
  circles->SetLines(lines);

  vtkSmartPointer<vtkPolyDataMapper> circleMapper
    = vtkSmartPointer<vtkPolyDataMapper>::New();
  circleMapper->SetInputData(circles);

  vtkSmartPointer<vtkActor> circleActor = vtkSmartPointer<vtkActor>::New();
  circleActor->SetMapper(circleMapper);

  vtkSmartPointer<vtkDecimatePolylineFilter> decimatePolylineFilter
    = vtkSmartPointer<vtkDecimatePolylineFilter>::New();
  decimatePolylineFilter->SetOutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION);
  decimatePolylineFilter->SetInputData(circles);
  decimatePolylineFilter->SetTargetReduction(0.9);
  decimatePolylineFilter->Update();

  if(decimatePolylineFilter->GetOutput()->GetPoints()->GetDataType() != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  decimatePolylineFilter->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);
  decimatePolylineFilter->Update();

  if(decimatePolylineFilter->GetOutput()->GetPoints()->GetDataType() != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  decimatePolylineFilter->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  decimatePolylineFilter->Update();

  if(decimatePolylineFilter->GetOutput()->GetPoints()->GetDataType() != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  vtkSmartPointer<vtkPolyDataMapper> decimatedMapper
    = vtkSmartPointer<vtkPolyDataMapper>::New();
  decimatedMapper->SetInputConnection(decimatePolylineFilter->GetOutputPort());

  vtkSmartPointer<vtkActor> decimatedActor
    = vtkSmartPointer<vtkActor>::New();
  decimatedActor->SetMapper(decimatedMapper);
  decimatedActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  vtkSmartPointer<vtkRenderer> renderer
    = vtkSmartPointer<vtkRenderer>::New();
  renderer->AddActor(circleActor);
  renderer->AddActor(decimatedActor);

  vtkSmartPointer<vtkRenderWindow> renderWindow
    = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  renderWindow->SetSize(300,300);

  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor
    = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
