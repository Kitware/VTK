/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAssignCoordinates.cxx

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

#include "vtkAssignCoordinates.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkGraphMapper.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSmartPointer.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestAssignCoordinates(int argc, char *argv[])
{
  cerr << "Generating graph ..." << endl;
  VTK_CREATE(vtkMutableDirectedGraph, g);
  VTK_CREATE(vtkDoubleArray, x);
  x->SetName("x");
  VTK_CREATE(vtkDoubleArray, y);
  y->SetName("y");
  VTK_CREATE(vtkDoubleArray, z);
  z->SetName("z");
  for (vtkIdType i = 0; i < 10; ++i)
    {
    for (vtkIdType j = 0; j < 10; ++j)
      {
      g->AddVertex();
      x->InsertNextValue(i);
      y->InsertNextValue(j);
      z->InsertNextValue(1);
      }
    }
  g->GetVertexData()->AddArray(x);
  g->GetVertexData()->AddArray(y);
  g->GetVertexData()->AddArray(z);
  cerr << "... done" << endl;

  cerr << "Sending graph through vtkAssignCoordinates ..." << endl;
  VTK_CREATE(vtkAssignCoordinates, assign);
  assign->SetInputData(g);
  assign->SetXCoordArrayName("x");
  assign->SetYCoordArrayName("y");
  assign->SetZCoordArrayName("z");
  assign->Update();
  cerr << "... done" << endl;

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

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Initialize();
    iren->Start();

    retVal = vtkRegressionTester::PASSED;
    }

  return !retVal;
}
