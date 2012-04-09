/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTreeRingView.cxx

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
#include "vtkRenderedTreeAreaRepresentation.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSplineGraphEdges.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"
#include "vtkTreeRingView.h"
#include "vtkViewTheme.h"
#include "vtkXMLTreeReader.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()
using std::string;

int TestTreeRingView(int argc, char* argv[])
{
  VTK_CREATE(vtkTesting, testHelper);
  testHelper->AddArguments(argc,const_cast<const char **>(argv));
  string dataRoot = testHelper->GetDataRoot();
  string treeFileName = dataRoot + "/Data/Infovis/XML/vtkclasses.xml";
  string graphFileName = dataRoot + "/Data/Infovis/XML/vtklibrary.xml";

  // We need to put the graph and tree edges in different domains.
  VTK_CREATE(vtkXMLTreeReader, reader1);
  reader1->SetFileName(treeFileName.c_str());
  reader1->SetEdgePedigreeIdArrayName("graph edge");
  reader1->GenerateVertexPedigreeIdsOff();
  reader1->SetVertexPedigreeIdArrayName("id");

  VTK_CREATE(vtkXMLTreeReader, reader2);
  reader2->SetFileName(graphFileName.c_str());
  reader2->SetEdgePedigreeIdArrayName("tree edge");
  reader2->GenerateVertexPedigreeIdsOff();
  reader2->SetVertexPedigreeIdArrayName("id");

  reader1->Update();
  reader2->Update();

  VTK_CREATE(vtkTreeRingView, view);
  view->DisplayHoverTextOn();
  view->SetTreeFromInputConnection(reader2->GetOutputPort());
  view->SetGraphFromInputConnection(reader1->GetOutputPort());
  view->Update();

  view->SetAreaColorArrayName("VertexDegree");

  // Uncomment for edge colors
  //view->SetEdgeColorArrayName("graph edge");
  //view->SetColorEdges(true);

  // Uncomment for edge labels
  //view->SetEdgeLabelArrayName("graph edge");
  //view->SetEdgeLabelVisibility(true);

  view->SetAreaLabelArrayName("id");
  view->SetAreaLabelVisibility(true);
  view->SetAreaHoverArrayName("id");
  view->SetAreaSizeArrayName("VertexDegree");
  vtkRenderedTreeAreaRepresentation::SafeDownCast(view->GetRepresentation())->SetGraphHoverArrayName("graph edge");
  vtkRenderedTreeAreaRepresentation::SafeDownCast(view->GetRepresentation())->SetGraphSplineType(vtkSplineGraphEdges::CUSTOM, 0);

  // Apply a theme to the views
  vtkViewTheme* const theme = vtkViewTheme::CreateMellowTheme();
  theme->SetLineWidth(1);
  theme->GetPointTextProperty()->ShadowOn();
  view->ApplyViewTheme(theme);
  theme->Delete();

  view->GetRenderWindow()->SetMultiSamples(0); // ensure to have the same test image everywhere
  view->ResetCamera();
  view->Render();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();

    retVal = vtkRegressionTester::PASSED;
    }

 return !retVal;
}


