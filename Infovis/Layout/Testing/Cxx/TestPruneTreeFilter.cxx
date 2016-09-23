/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPruneTreeFilter.cxx

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
#include "vtkActor2D.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraphLayout.h"
#include "vtkGraphMapper.h"
#include "vtkGraphToPolyData.h"
#include "vtkIdTypeArray.h"
#include "vtkLabeledDataMapper.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkPruneTreeFilter.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTreeLayoutStrategy.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestPruneTreeFilter(int argc, char* argv[])
{
  VTK_CREATE(vtkMutableDirectedGraph, builder);
  builder->AddVertex(); // 0
  builder->AddChild(0); // 1
  builder->AddChild(0); // 2
  builder->AddChild(1); // 3
  builder->AddChild(1); // 4
  builder->AddChild(2); // 5
  builder->AddChild(2); // 6
  builder->AddChild(3); // 7
  builder->AddChild(3); // 8
  builder->AddChild(4); // 9
  builder->AddChild(4); // 10
  VTK_CREATE(vtkTree, tree);
  tree->ShallowCopy(builder);

  VTK_CREATE(vtkIdTypeArray, idArr);
  idArr->SetName("id");
  for (vtkIdType i = 0; i < 11; ++i)
  {
    idArr->InsertNextValue(i);
  }
  tree->GetVertexData()->AddArray(idArr);

  VTK_CREATE(vtkPruneTreeFilter, prune);
  prune->SetInputData(tree);
  prune->SetParentVertex(2);

  VTK_CREATE(vtkTreeLayoutStrategy, strategy);
  VTK_CREATE(vtkGraphLayout, layout);
  //layout->SetInput(tree);
  layout->SetInputConnection(prune->GetOutputPort());
  layout->SetLayoutStrategy(strategy);

  VTK_CREATE(vtkGraphToPolyData, poly);
  poly->SetInputConnection(layout->GetOutputPort());
  VTK_CREATE(vtkLabeledDataMapper, labelMapper);
  labelMapper->SetInputConnection(poly->GetOutputPort());
  labelMapper->SetLabelModeToLabelFieldData();
  labelMapper->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "id");
  VTK_CREATE(vtkActor2D, labelActor);
  labelActor->SetMapper(labelMapper);

  VTK_CREATE(vtkGraphMapper, mapper);
  mapper->SetInputConnection(layout->GetOutputPort());
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);
  VTK_CREATE(vtkRenderer, ren);
  ren->AddActor(actor);
  ren->AddActor(labelActor);
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
