// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkDataRepresentation.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"
#include "vtkTreeRingView.h"
#include "vtkViewTheme.h"
#include "vtkXMLTreeReader.h"

#include <QApplication>
#include <QFontDatabase>

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()
using std::string;

int TestQtTreeRingLabeler(int argc, char* argv[])
{
  VTK_CREATE(vtkTesting, testHelper);
  testHelper->AddArguments(argc, const_cast<const char**>(argv));
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

  view->GetRenderWindow()->SetSize(600, 600);
  view->GetRenderWindow()->SetMultiSamples(0); // ensure to have the same test image everywhere
  view->ResetCamera();
  view->Render();

  // using image-test threshold of 200 since this test tends to render slightly
  // differently on different platforms.
  int retVal = vtkRegressionTestImageThreshold(view->GetRenderWindow(), 0.05);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();

    retVal = vtkRegressionTester::PASSED;
  }

  QFontDatabase::removeAllApplicationFonts();

  return !retVal;
}
