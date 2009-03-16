/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTreeRingView3.cxx

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
#include "vtkRenderWindow.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkTreeRingView3.h"
#include "vtkViewTheme.h"
#include "vtkXMLTreeReader.h"
#include "vtkQtInitialization.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()
using vtkstd::string;

int TestTreeRingView3(int argc, char* argv[])
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
  reader1->GenerateVertexPedigreeIdsOff();
  reader1->SetVertexPedigreeIdArrayName("id");

  VTK_CREATE(vtkXMLTreeReader, reader2);
  reader2->SetFileName(graphFileName.c_str());
  reader2->SetEdgePedigreeIdArrayName("graph edge");
  reader2->GenerateVertexPedigreeIdsOff();
  reader2->SetVertexPedigreeIdArrayName("id");

  reader1->Update();
  reader2->Update();

  VTK_CREATE(vtkTreeRingView3, view);
  view->SetTreeFromInputConnection(reader2->GetOutputPort());
  view->SetGraphFromInputConnection(reader1->GetOutputPort());

  view->SetAreaColorArrayName("GraphVertexDegree");
  //view->SetEdgeColorArrayName("tree edge");
  view->SetEdgeColorToSplineFraction();
  view->SetColorEdges(true);
  view->SetAreaLabelArrayName("id");
  view->SetAreaHoverArrayName("id");
  view->SetAreaLabelVisibility(true);
  view->SetAreaSizeArrayName("GraphVertexDegree");

  // Apply a theme to the views
  vtkViewTheme* const theme = vtkViewTheme::CreateMellowTheme();
  view->ApplyViewTheme(theme);
  theme->Delete();

  VTK_CREATE(vtkRenderWindow, win);
  win->SetSize(512, 512); 
  win->SetMultiSamples(0); // ensure to have the same test image everywhere
  view->SetupRenderWindow(win);
  view->Update();

  int retVal = vtkRegressionTestImage(win);
//  if( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    win->GetInteractor()->Initialize();
    win->GetInteractor()->Start();

    retVal = vtkRegressionTester::PASSED;
    }

 return !retVal;
}


