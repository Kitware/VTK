/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDirectSelectionRendering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers selection through the polydata mapper

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSphereSource.h"

//----------------------------------------------------------------------------
int TestDirectSelectionRendering(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkRenderer> renderer;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkSphereSource> sphere;
  sphere->Update();

  vtkPolyData* pd = vtkPolyData::SafeDownCast(sphere->GetOutput());
  vtkIdType nbPolys = pd->GetNumberOfPolys();

  // generate a value array
  vtkNew<vtkIdTypeArray> idArray;
  idArray->SetNumberOfTuples(nbPolys);
  idArray->SetName("Odd");
  for (vtkIdType i = 0; i < nbPolys; i++)
  {
    idArray->SetTypedComponent(i, 0, i % 4);
  }

  pd->GetCellData()->AddArray(idArray);

  // selection by id
  vtkNew<vtkPolyDataMapper> pdSphere;
  pdSphere->SetInputData(pd);

  vtkNew<vtkSelection> selection;
  vtkNew<vtkSelectionNode> selectionNode;
  selection->AddNode(selectionNode);

  vtkNew<vtkIdTypeArray> selectionArray;
  selectionArray->SetNumberOfTuples(2);
  selectionArray->SetTypedComponent(0, 0, 0);
  selectionArray->SetTypedComponent(1, 0, 3);

  vtkNew<vtkDataSetAttributes> selectionAttr;
  selectionAttr->AddArray(selectionArray);

  selectionNode->SetSelectionData(selectionAttr);
  selectionNode->SetFieldType(vtkSelectionNode::SelectionField::CELL);
  selectionNode->SetContentType(vtkSelectionNode::SelectionContent::INDICES);

  pdSphere->SetSelection(selection);

  vtkNew<vtkActor> actorSphere;
  actorSphere->GetProperty()->SetSelectionColor(0.0, 0.0, 1.0, 1.0);
  actorSphere->GetProperty()->SetSelectionLineWidth(3.0);
  actorSphere->SetMapper(pdSphere);
  renderer->AddActor(actorSphere);

  // selection by value
  vtkNew<vtkPolyDataMapper> pdSphereVal;
  pdSphereVal->SetInputData(pd);

  vtkNew<vtkSelection> selectionVal;
  vtkNew<vtkSelectionNode> selectionNodeVal;
  selectionVal->AddNode(selectionNodeVal);

  vtkNew<vtkIdTypeArray> selectionArrayVal;
  selectionArrayVal->SetNumberOfTuples(1);
  selectionArrayVal->SetTypedComponent(0, 0, 0);
  selectionArrayVal->SetName("Odd");

  vtkNew<vtkDataSetAttributes> selectionAttrVal;
  selectionAttrVal->AddArray(selectionArrayVal);

  selectionNodeVal->SetSelectionData(selectionAttrVal);
  selectionNodeVal->SetFieldType(vtkSelectionNode::SelectionField::CELL);
  selectionNodeVal->SetContentType(vtkSelectionNode::SelectionContent::VALUES);

  pdSphereVal->SetSelection(selectionVal);

  vtkNew<vtkActor> actorSphereVal;
  actorSphereVal->SetPosition(1, 0, 0);
  actorSphereVal->GetProperty()->SetSelectionColor(0.0, 0.0, 0.0, 0.2);
  actorSphere->GetProperty()->SetSelectionLineWidth(1.0);
  actorSphereVal->SetMapper(pdSphereVal);
  renderer->AddActor(actorSphereVal);

  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
