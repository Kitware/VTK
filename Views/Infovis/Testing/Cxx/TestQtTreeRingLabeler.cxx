/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtTreeRingLabeler.cxx

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
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"
#include "vtkTreeRingView.h"
#include "vtkViewTheme.h"
#include "vtkXMLTreeReader.h"

#include <QApplication>
#include <QFontDatabase>

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()
using std::string;

int TestQtTreeRingLabeler(int argc, char* argv[])
{
  VTK_CREATE(vtkTesting, testHelper);
  testHelper->AddArguments(argc,const_cast<const char **>(argv));
  string dataRoot = testHelper->GetDataRoot();
  string treeFileName = dataRoot + "/Data/Infovis/XML/vtklibrary.xml";

  VTK_CREATE(vtkXMLTreeReader, reader);
  reader->SetFileName(treeFileName.c_str());
  reader->SetEdgePedigreeIdArrayName("graph edge");
  reader->GenerateVertexPedigreeIdsOff();
  reader->SetVertexPedigreeIdArrayName("id");

  reader->Update();

  QApplication app(argc, argv);

  QString fontFileName = testHelper->GetDataRoot();
  fontFileName.append("/Data/Infovis/martyb_-_Ridiculous.ttf");
  QFontDatabase::addApplicationFont(fontFileName);

  VTK_CREATE(vtkTreeRingView, view);
  view->SetTreeFromInputConnection(reader->GetOutputPort());
  view->Update();
  view->SetLabelRenderModeToQt();
  view->SetAreaColorArrayName("VertexDegree");
  view->SetEdgeColorToSplineFraction();
  view->SetColorEdges(true);
  view->SetAreaLabelArrayName("id");
  view->SetAreaHoverArrayName("id");
  view->SetAreaLabelVisibility(true);
  view->SetAreaSizeArrayName("VertexDegree");

  // Apply a theme to the views
  vtkViewTheme* const theme = vtkViewTheme::CreateMellowTheme();
//  theme->GetPointTextProperty()->SetColor(0, 0, 0);
  theme->GetPointTextProperty()->SetFontFamilyAsString("Ridiculous");
  theme->GetPointTextProperty()->BoldOn();
  theme->GetPointTextProperty()->SetFontSize(16);
  theme->GetPointTextProperty()->ShadowOn();
  view->ApplyViewTheme(theme);
  theme->Delete();

  view->GetRenderWindow()->SetSize(600,600);
  view->GetRenderWindow()->SetMultiSamples(0); // ensure to have the same test image everywhere
  view->ResetCamera();
  view->Render();

  // using image-test threshold of 200 since this test tends to render slightly
  // differently on different platforms.
  int retVal = vtkRegressionTestImageThreshold(view->GetRenderWindow(), 200);
  if( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();

    retVal = vtkRegressionTester::PASSED;
    }

  QFontDatabase::removeAllApplicationFonts();

  return !retVal;
}


