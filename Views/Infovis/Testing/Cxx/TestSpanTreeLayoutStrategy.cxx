// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkGraphLayoutView.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSpanTreeLayoutStrategy.h"
#include "vtkTestUtilities.h"
#include "vtkXGMLReader.h"

using std::string;

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestSpanTreeLayoutStrategy(int argc, char* argv[])
{
  VTK_CREATE(vtkTesting, testHelper);
  testHelper->AddArguments(argc, const_cast<const char**>(argv));
  string dataRoot = testHelper->GetDataRoot();
  string file = dataRoot + "/Data/Infovis/fsm.gml";

  VTK_CREATE(vtkXGMLReader, reader);
  reader->SetFileName(file.c_str());
  reader->Update();

  // Graph layout view
  VTK_CREATE(vtkGraphLayoutView, view);
  view->DisplayHoverTextOff();
  view->SetLayoutStrategyToSpanTree();
  view->SetVertexLabelArrayName("vertex id");
  view->VertexLabelVisibilityOn();
  view->SetVertexColorArrayName("vertex id");
  view->SetColorVertices(true);
  view->SetRepresentationFromInputConnection(reader->GetOutputPort());

  view->ResetCamera();
  view->GetRenderWindow()->SetSize(600, 600);
  view->GetRenderWindow()->SetMultiSamples(0); // ensure to have the same test image everywhere
  view->SetInteractionModeTo3D();
  view->SetLabelPlacementModeToNoOverlap();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();

    retVal = vtkRegressionTester::PASSED;
  }

  return !retVal;
}
