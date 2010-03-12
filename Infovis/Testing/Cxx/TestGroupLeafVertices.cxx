/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGroupLeafVertices.cxx

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
//#include "vtkDynamic2DLabelMapper.h"
#include "vtkGlyph3D.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToPolyData.h"
#include "vtkGroupLeafVertices.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTableToTreeFilter.h"
#include "vtkTree.h"
#include "vtkTreeLayoutStrategy.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestGroupLeafVertices(int argc, char* argv[])
{
  int imode = 0; // Interactive mode
  for(int i = 1; i < argc; i++)
    {
    if (!strcmp(argv[i], "-I")) 
      {
      imode = 1;
      continue;
      }

    cerr << argv[0] << " Options:\n  " 
      << " -h (prints this message)\n  "
      << " -I (run interactively)\n  ";
    return 0;
    }

  VTK_CREATE(vtkTable, table);
  VTK_CREATE(vtkStringArray, nameArray);
  nameArray->SetName("name");
  VTK_CREATE(vtkStringArray, typeArray);
  typeArray->SetName("type");
  VTK_CREATE(vtkStringArray, colorArray);
  colorArray->SetName("color");

  nameArray->InsertNextValue("bobo");
  typeArray->InsertNextValue("dog");
  colorArray->InsertNextValue("black");

  nameArray->InsertNextValue("rex");
  typeArray->InsertNextValue("dog");
  colorArray->InsertNextValue("brown");

  nameArray->InsertNextValue("bill");
  typeArray->InsertNextValue("cat");
  colorArray->InsertNextValue("black");

  nameArray->InsertNextValue("eli");
  typeArray->InsertNextValue("dog");
  colorArray->InsertNextValue("black");

  nameArray->InsertNextValue("spot");
  typeArray->InsertNextValue("dog");
  colorArray->InsertNextValue("white");

  nameArray->InsertNextValue("roger");
  typeArray->InsertNextValue("cat");
  colorArray->InsertNextValue("black");

  nameArray->InsertNextValue("tweety");
  typeArray->InsertNextValue("bird");
  colorArray->InsertNextValue("white");

  nameArray->InsertNextValue("mike");
  typeArray->InsertNextValue("bird");
  colorArray->InsertNextValue("brown");

  nameArray->InsertNextValue("spike");
  typeArray->InsertNextValue("dog");
  colorArray->InsertNextValue("brown");

  table->AddColumn(nameArray);
  table->AddColumn(typeArray);
  table->AddColumn(colorArray);

  //
  // Create a tree from the table
  //

  VTK_CREATE(vtkTableToTreeFilter, tableToTree);
  tableToTree->SetInput(table);
  tableToTree->Update();
  vtkTree* tree = tableToTree->GetOutput();
  for (vtkIdType i = 0; i < tree->GetNumberOfVertices(); i++)
    {
    cerr << i << " has parent " << tree->GetParent(i) << endl;
    }

  VTK_CREATE(vtkGroupLeafVertices, group);
  group->SetInputConnection(tableToTree->GetOutputPort());
  group->SetInputArrayToProcess(0, 0, 0, vtkDataSet::FIELD_ASSOCIATION_VERTICES, "type");
  group->SetInputArrayToProcess(1, 0, 0, vtkDataSet::FIELD_ASSOCIATION_VERTICES, "name");
  group->Update();
  tree = group->GetOutput();
  for (vtkIdType i = 0; i < tree->GetNumberOfVertices(); i++)
    {
    cerr << i << " has parent " << tree->GetParent(i) << endl;
    }

  VTK_CREATE(vtkGroupLeafVertices, group2);
  group2->SetInputConnection(group->GetOutputPort());
  group2->SetInputArrayToProcess(0, 0, 0, vtkDataSet::FIELD_ASSOCIATION_VERTICES, "color");
  group2->SetInputArrayToProcess(1, 0, 0, vtkDataSet::FIELD_ASSOCIATION_VERTICES, "name");
  group2->Update();
  tree = group2->GetOutput();
  for (vtkIdType i = 0; i < tree->GetNumberOfVertices(); i++)
    {
    cerr << i << " has parent " << tree->GetParent(i) << endl;
    }

  //
  // Render the tree
  //

  VTK_CREATE(vtkTreeLayoutStrategy, strategy);
  strategy->SetRadial(true);
  strategy->SetAngle(360);

  VTK_CREATE(vtkGraphLayout, layout);
  layout->SetInputConnection(group2->GetOutputPort());
  layout->SetLayoutStrategy(strategy);

  VTK_CREATE(vtkGraphToPolyData, graphToPoly);
  graphToPoly->SetInputConnection(layout->GetOutputPort());

  VTK_CREATE(vtkPolyDataMapper, polyMapper);
  polyMapper->SetInputConnection(graphToPoly->GetOutputPort());

  VTK_CREATE(vtkActor, polyActor);
  polyActor->SetMapper(polyMapper);
  polyActor->GetProperty()->SetColor(0.3, 0.3, 1.0);

  //
  // Make some labels
  //

#if 0
  VTK_CREATE(vtkDynamic2DLabelMapper, labelMapper);
  labelMapper->SetInputConnection(graphToPoly->GetOutputPort());
  labelMapper->SetLabelFormat("%s");
  labelMapper->SetLabelModeToLabelFieldData();
  labelMapper->SetFieldDataName("name");

  VTK_CREATE(vtkActor2D, labelActor);
  labelActor->SetMapper(labelMapper);
#endif

  //
  // Make some glyphs
  //

  VTK_CREATE(vtkSphereSource, sphere);
  sphere->SetRadius(0.05);
  sphere->SetPhiResolution(6);
  sphere->SetThetaResolution(6);

  VTK_CREATE(vtkGlyph3D, glyph);
  glyph->SetInputConnection(0, graphToPoly->GetOutputPort());
  glyph->SetInputConnection(1, sphere->GetOutputPort());

  VTK_CREATE(vtkPolyDataMapper, glyphMap);
  glyphMap->SetInputConnection(glyph->GetOutputPort());

  VTK_CREATE(vtkActor, glyphActor);
  glyphActor->SetMapper(glyphMap);
  glyphActor->GetProperty()->SetColor(0.3, 0.3, 1.0);

  //
  // Set up the main window
  //

  VTK_CREATE(vtkRenderer, ren);
  ren->AddActor(polyActor);
  //ren->AddActor(labelActor);
  ren->AddActor(glyphActor);

  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);

  if (imode)
    {
    iren->Initialize();
    iren->Start();
    }

  return 0;
}
