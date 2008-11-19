/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHierarchicalTreeRingView.cxx
  
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
#include "vtkHierarchicalTreeRingView.h"
#include "vtkRenderWindow.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkTestUtilities.h"
#include "vtkViewTheme.h"
#include "vtkXMLTreeReader.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

using vtkstd::string;

int TestHierarchicalTreeRingView(int argc, char* argv[]) 
{
  VTK_CREATE(vtkTesting, testHelper);
  testHelper->AddArguments(argc,const_cast<const char **>(argv));
  string dataRoot = testHelper->GetDataRoot();
  string treeFileName = dataRoot + "/Data/Infovis/XML/vtkclasses.xml";
  string graphFileName = dataRoot + "/Data/Infovis/XML/vtklibrary.xml";

  // We need to put the graph and tree edges in different domains.
  VTK_CREATE(vtkXMLTreeReader, reader1);
  reader1->SetFileName(treeFileName.c_str());
  reader1->SetEdgePedigreeIdArrayName("tree edge");
  
  VTK_CREATE(vtkXMLTreeReader, reader2);
  reader2->SetFileName(graphFileName.c_str());
  reader2->SetEdgePedigreeIdArrayName("graph edge");

  reader1->Update();
  reader2->Update();
  
  VTK_CREATE(vtkHierarchicalTreeRingView, dummy);
  VTK_CREATE(vtkHierarchicalTreeRingView, view);
  view->SetHierarchyFromInputConnection(reader2->GetOutputPort());
  view->SetGraphFromInputConnection(reader1->GetOutputPort());

//  view->SetVertexColorArrayName("VertexDegree");
  view->SetVertexColorArrayName("vertex id");
  view->SetEdgeColorArrayName("tree edge");
  view->SetColorEdges(true);
  view->SetVertexLabelArrayName("id");
  view->SetHoverArrayName("id");
  view->SetVertexLabelVisibility(true);
  
  // Apply a theme to the views
  vtkViewTheme* const theme = vtkViewTheme::CreateMellowTheme();
  view->ApplyViewTheme(theme);
  theme->Delete();
 
  VTK_CREATE(vtkRenderWindow, win);
  dummy->SetupRenderWindow(win);
  view->SetupRenderWindow(win);
  
  int retVal = vtkRegressionTestImage(win);
//  if( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    win->GetInteractor()->Initialize();
    win->GetInteractor()->Start();

    retVal = vtkRegressionTester::PASSED;
    }
  
 return 0;
}


