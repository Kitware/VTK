/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkSmartPointer.h>
#include <vtkTextSource.h>

int TestTextSource(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkTextSource> textSource
    = vtkSmartPointer<vtkTextSource>::New();
  textSource->SetBackgroundColor(0.0, 0.0, 0.0);
  textSource->SetForegroundColor(1.0, 1.0, 1.0);
  textSource->BackingOn();

  textSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  textSource->SetText("1234567890abcdefghijklmnopqrstuvwxyz");
  textSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = textSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  textSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  textSource->SetText("1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  textSource->Update();

  polyData = textSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
