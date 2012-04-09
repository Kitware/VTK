/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTreeMapLayoutStrategy.cxx

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

#include "vtkActor.h"
#include "vtkBoxLayoutStrategy.h"
#include "vtkIntArray.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSliceAndDiceLayoutStrategy.h"
#include "vtkSmartPointer.h"
#include "vtkSquarifyLayoutStrategy.h"
#include "vtkTestUtilities.h"
#include "vtkTree.h"
#include "vtkTreeFieldAggregator.h"
#include "vtkTreeMapLayout.h"
#include "vtkTreeMapToPolyData.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


void TestStrategy(vtkTreeMapLayoutStrategy* strategy, vtkTreeAlgorithm* input, double posX, double posY, vtkRenderer* ren)
{
  strategy->SetShrinkPercentage(0.1);
  VTK_CREATE(vtkTreeMapLayout, layout);
  layout->SetLayoutStrategy(strategy);
  layout->SetInputConnection(input->GetOutputPort());
  VTK_CREATE(vtkTreeMapToPolyData, poly);
  poly->SetInputConnection(layout->GetOutputPort());
  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(poly->GetOutputPort());
  mapper->SetScalarRange(0, 100);
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("size");
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);
  actor->SetPosition(posX, posY, 0);
  ren->AddActor(actor);
}

int TestTreeMapLayoutStrategy(int argc, char* argv[])
{
  VTK_CREATE(vtkRenderer, ren);

  // Create input
  VTK_CREATE(vtkMutableDirectedGraph, builder);
  VTK_CREATE(vtkIntArray, sizeArr);
  sizeArr->SetName("size");
  builder->GetVertexData()->AddArray(sizeArr);
  builder->AddVertex();
  sizeArr->InsertNextValue(0);
  builder->AddChild(0);
  sizeArr->InsertNextValue(15);
  builder->AddChild(0);
  sizeArr->InsertNextValue(50);
  builder->AddChild(0);
  sizeArr->InsertNextValue(0);
  builder->AddChild(3);
  sizeArr->InsertNextValue(2);
  builder->AddChild(3);
  sizeArr->InsertNextValue(12);
  builder->AddChild(3);
  sizeArr->InsertNextValue(10);
  builder->AddChild(3);
  sizeArr->InsertNextValue(8);
  builder->AddChild(3);
  sizeArr->InsertNextValue(6);
  builder->AddChild(3);
  sizeArr->InsertNextValue(4);

  VTK_CREATE(vtkTree, tree);
  if (!tree->CheckedShallowCopy(builder))
    {
    cerr << "Invalid tree structure." << endl;
    }

  VTK_CREATE(vtkTreeFieldAggregator, agg);
  agg->SetInputData(tree);
  agg->SetField("size");
  agg->SetLeafVertexUnitSize(false);

  // Test box layout
  VTK_CREATE(vtkBoxLayoutStrategy, box);
  TestStrategy(box, agg, 0, 0, ren);

  // Test slice and dice layout
  VTK_CREATE(vtkSliceAndDiceLayoutStrategy, sd);
  TestStrategy(sd, agg, 0, 1.1, ren);

  // Test squarify layout
  VTK_CREATE(vtkSquarifyLayoutStrategy, sq);
  TestStrategy(sq, agg, 1.1, 0, ren);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    win->Render();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
    }
  return !retVal;
}
