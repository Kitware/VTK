/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHierarchicalGraphView.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataRepresentation.h"
#include "vtkHierarchicalGraphView.h"
#include "vtkRenderWindow.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkViewTheme.h"
#include "vtkXMLTreeReader.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestHierarchicalGraphView(int argc, char* argv[]) 
{
  char* treeFileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                 "Data/Infovis/XML/vtkclasses.xml");
  char* graphFileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                 "Data/Infovis/XML/vtklibrary.xml");

  VTK_CREATE(vtkXMLTreeReader, reader1);
  reader1->SetFileName(treeFileName);
  
  VTK_CREATE(vtkXMLTreeReader, reader2);
  reader2->SetFileName(graphFileName);

  reader1->Update();
  reader2->Update();
  
  VTK_CREATE(vtkHierarchicalGraphView, view);
  view->SetHierarchyFromInputConnection(reader2->GetOutputPort());
  view->SetGraphFromInputConnection(reader1->GetOutputPort());

  view->SetVertexColorArrayName("VertexDegree");
  view->SetColorVertices(true);
//  #view->SetEdgeColorArrayName("weight");
  view->SetColorEdges(true);
  view->SetVertexLabelArrayName("id");
  view->SetVertexLabelVisibility(true);
  view->SetLayoutStrategyToCosmicTree();
  view->SetScalingArrayName("VertexDegree");
  
  VTK_CREATE(vtkHierarchicalGraphView, view2);
  view2->SetRepresentationFromInputConnection(reader2->GetOutputPort());
  view2->SetRepresentationFromInputConnection(1, reader1->GetOutputPort());

  view2->SetVertexColorArrayName("VertexDegree");
  view2->SetColorVertices(true);
//  #view2->SetEdgeColorArrayName("weight");
  view2->SetColorEdges(true);
  view2->SetVertexLabelArrayName("id");
  view2->SetVertexLabelVisibility(true);
  
    //# Apply a theme to the views
  vtkViewTheme* const theme = vtkViewTheme::CreateMellowTheme();
  view->ApplyViewTheme(theme);
  view2->ApplyViewTheme(theme);
  theme->Delete();
 
  VTK_CREATE(vtkRenderWindow, win);
  view->SetupRenderWindow(win);
  
  VTK_CREATE(vtkRenderWindow, win2);
  view2->SetupRenderWindow(win2);
  
  int retVal = vtkRegressionTestImage(win);
  if( retVal == vtkRegressionTester::DO_INTERACTOR )
  {
    win->GetInteractor()->Initialize();
    win2->GetInteractor()->Initialize();
    
    win->GetInteractor()->Start();

   retVal = vtkRegressionTester::PASSED;
 }
  
 return 0;
}


