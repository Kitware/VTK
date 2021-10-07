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
#include "vtkCirclePackFrontChainLayoutStrategy.h"
#include "vtkCirclePackLayout.h"
#include "vtkCirclePackToPolyData.h"
#include "vtkIntArray.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTree.h"
#include "vtkTreeFieldAggregator.h"

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

void TestStrategy(vtkCirclePackLayoutStrategy* strategy, vtkTreeAlgorithm* input, double posX,
  double posY, vtkRenderer* ren)
{
  VTK_CREATE(vtkCirclePackLayout, layout);
  layout->SetLayoutStrategy(strategy);
  layout->SetInputConnection(input->GetOutputPort());
  layout->Update();
  vtkDataArray* vda = layout->GetOutput()->GetVertexData()->GetArray("circles");
  // Test GetBoundingCircle() and FindVertex()
  double cinfo[3];
  layout->GetBoundingCircle(vda->GetNumberOfTuples() - 1, cinfo);
  double pnt[2];
  pnt[0] = cinfo[0];
  pnt[1] = cinfo[1];
  if (((int)layout->FindVertex(pnt)) != (vda->GetNumberOfTuples() - 1))
  {
    cout << "GetBoundingCircle() and FindVertex() returned incorrect id" << endl;
    exit(1);
  }

  VTK_CREATE(vtkCirclePackToPolyData, poly);
  poly->SetInputConnection(layout->GetOutputPort());
  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(poly->GetOutputPort());
  mapper->SetScalarRange(0, 600);
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("size");
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);
  actor->SetPosition(posX, posY, 0);
  ren->AddActor(actor);
}

const int values[] = { 1, 100, 1, 400, 500, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  400, 1, 100, 1, 400, 500, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 100, 1, 400, 500, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 400, 1, 100, 1, 400, 500, 1, 1, 1, 1, 77, 1, 1, 1, 1, 1, 1,
  100, 1, 400, 500, 1, 1, 1, 1, 1, 15, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 400, 1, 100, 1, 400,
  500, 1, 1, 1, 1, 99, 1, 1, 1, 1, 1, 1, 100, 1, 400, 500, 1, 1, 1, 1, 1, 1, 107, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 432, 1, 100, 1, 400, 500, 1, 1, 259, 1, 1, 1, 1, 1, 1, 242, 1, 100, 306, 400,
  500, 1, 1, 1, 1, 1, 1, 91, 1, 1, 46, 1, 1, 1, 1, 1, 1, 1, 1, 1, 400, 1, 100, 1, 400, 500, 1, 1, 1,
  1, 1, 47, 1, 1, 1, 1, 1, 100, 1, 400, 500, 1, 1, 1, 150, 1, 90, 1, 1, 1, 1, 10, 1, 1, 456, 1, 1,
  1, 1, 1, 40, 1, 100, 1, 400, 500, 1, 1, 1, 1, 1, 1, 1, 98, 1, 1, 1, 100, 1, 400, 500, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 105, 1, 1, 1, 15, 1, 1, 1, 410, 1, 320, 1, 410, 450, 1, 1, 136, 1, 1, 1, 1,
  458, 1, 1 };

int TestCirclePackLayoutStrategy(int argc, char* argv[])
{
  VTK_CREATE(vtkRenderer, ren);
  // Create input
  VTK_CREATE(vtkMutableDirectedGraph, builder);
  VTK_CREATE(vtkIntArray, sizeArr);
  sizeArr->SetName("size");
  builder->GetVertexData()->AddArray(sizeArr);
  builder->AddVertex();
  sizeArr->InsertNextValue(0);
  for (auto value : values)
  {
    builder->AddChild(0);
    sizeArr->InsertNextValue(value);
  }

  VTK_CREATE(vtkTree, tree);
  if (!tree->CheckedShallowCopy(builder))
  {
    cerr << "Invalid tree structure." << endl;
  }

  VTK_CREATE(vtkTreeFieldAggregator, agg);
  agg->SetInputData(tree);
  agg->SetField("size");
  agg->SetLeafVertexUnitSize(false);

  // Test Front Chain layout
  VTK_CREATE(vtkCirclePackFrontChainLayoutStrategy, fc);
  TestStrategy(fc, agg, 0, 0, ren);

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
