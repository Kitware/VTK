/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGeoAssignCoordinates.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkGeoAssignCoordinates.h"
#include "vtkGraphMapper.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestGeoAssignCoordinates(int argc, char *argv[])
{
  VTK_CREATE(vtkMutableDirectedGraph, g);
  VTK_CREATE(vtkDoubleArray, latitude);
  latitude->SetName("latitude");
  VTK_CREATE(vtkDoubleArray, longitude);
  longitude->SetName("longitude");
  for (vtkIdType i = -90; i <= 90; i += 10)
    {
    for (vtkIdType j = -180; j < 180; j += 20)
      {
      g->AddVertex();
      latitude->InsertNextValue(i);
      longitude->InsertNextValue(j);
      }
    }
  g->GetVertexData()->AddArray(latitude);
  g->GetVertexData()->AddArray(longitude);

  VTK_CREATE(vtkGeoAssignCoordinates, assign);
  assign->SetInput(g);
  assign->SetLatitudeArrayName("latitude");
  assign->SetLongitudeArrayName("longitude");
  assign->SetGlobeRadius(1.0);
  assign->Update();

  VTK_CREATE(vtkGraphMapper, mapper);
  mapper->SetInputConnection(assign->GetOutputPort());
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);
  VTK_CREATE(vtkRenderer, ren);
  ren->AddActor(actor);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);
  ren->ResetCamera();

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Initialize();
    iren->Start();
    
    retVal = vtkRegressionTester::PASSED;
    }

  return !retVal;
}
