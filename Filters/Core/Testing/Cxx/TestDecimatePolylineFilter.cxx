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
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>

int TestDecimatePolylineFilter(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  const unsigned int numberOfPoints = 100;

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataType(VTK_FLOAT);

  vtkIdType *lineIds = new vtkIdType[numberOfPoints+1];

  for(unsigned int i = 0; i < numberOfPoints; ++i)
    {
    const double angle = 2.0 * vtkMath::Pi() * static_cast<double>(i)
      / static_cast<double>(numberOfPoints);
    points->InsertPoint(static_cast<vtkIdType>(i), std::cos(angle),
      std::sin(angle), 0.0);
    lineIds[i] = static_cast<vtkIdType>(i);
    }

  lineIds[numberOfPoints] = 0;

  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  lines->InsertNextCell(numberOfPoints + 1, lineIds);
  delete[] lineIds;

  vtkSmartPointer<vtkPolyData> circle = vtkSmartPointer<vtkPolyData>::New();
  circle->SetPoints(points);
  circle->SetLines(lines);

  vtkSmartPointer<vtkPolyDataMapper> circleMapper
    = vtkSmartPointer<vtkPolyDataMapper>::New();
  circleMapper->SetInputData(circle);

  vtkSmartPointer<vtkActor> circleActor = vtkSmartPointer<vtkActor>::New();
  circleActor->SetMapper(circleMapper);

  vtkSmartPointer<vtkDecimatePolylineFilter> decimatePolylineFilter
    = vtkSmartPointer<vtkDecimatePolylineFilter>::New();
  decimatePolylineFilter->SetOutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION);
  decimatePolylineFilter->SetInputData(circle);
  decimatePolylineFilter->SetTargetReduction(0.95);
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

  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor
    = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderWindow->Render();
  renderWindowInteractor->CreateOneShotTimer(1);

  return EXIT_SUCCESS;
}
