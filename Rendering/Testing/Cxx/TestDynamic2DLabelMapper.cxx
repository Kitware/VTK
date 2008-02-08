/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDynamic2DLabelMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests vtkDynamic2DLabelMapper 

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkCellArray.h"
#include "vtkDynamic2DLabelMapper.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTestUtilities.h"
#include "vtkVariant.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestDynamic2DLabelMapper(int argc, char* argv[])
{
  vtkIdType numPoints = 75;
  
  VTK_CREATE(vtkPolyData, poly);
  VTK_CREATE(vtkPoints, pts);
  VTK_CREATE(vtkCellArray, cells);
  cells->Allocate(cells->EstimateSize(numPoints ,1));
  pts->SetNumberOfPoints(numPoints);
  double x[3];
  for (vtkIdType i = 0; i < numPoints; ++i)
    {
    double v = 20.0 * static_cast<double>(i) / numPoints;
    x[0] = v*cos(v);
    x[1] = v*sin(v);
    x[2] = 0;
    pts->SetPoint(i, x);
    
    cells->InsertNextCell(1, &i);
    }
  poly->SetPoints(pts);
  poly->SetVerts(cells);

  VTK_CREATE(vtkStringArray, nameArray);
  nameArray->SetName("name");
  for (vtkIdType i = 0; i < numPoints; i++)
    {
    nameArray->InsertNextValue(vtkVariant(i).ToString());
    }
  poly->GetPointData()->AddArray(nameArray);

  VTK_CREATE(vtkDynamic2DLabelMapper, mapper);
  mapper->SetInput(poly);
  VTK_CREATE(vtkActor2D, actor);
  actor->SetMapper(mapper);
  
  VTK_CREATE(vtkPolyDataMapper, polyMapper);
  polyMapper->SetInput(poly);
  VTK_CREATE(vtkActor, polyActor);
  polyActor->SetMapper(polyMapper);
  
  VTK_CREATE(vtkRenderer, ren);
  ren->AddActor(actor);
  ren->AddActor(polyActor);
  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);
  
  ren->ResetCamera();
  win->Render();
  
  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Initialize();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
    }
    
  return !retVal;
}
