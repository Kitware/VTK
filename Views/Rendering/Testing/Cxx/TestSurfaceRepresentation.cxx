// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <iostream>

#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkStandardRenderView.h"
#include "vtkSurfaceRepresentation.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

int TestSurfaceRepresentation(int argc, char* argv[])
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(32);
  sphere->SetPhiResolution(32);

  vtkNew<vtkSurfaceRepresentation> rep;
  rep->SetInputConnection(sphere->GetOutputPort());

  // Test representation modes
  rep->SetRepresentationToSurfaceWithEdges();
  if (rep->GetRepresentation() != vtkSurfaceRepresentation::SURFACE_WITH_EDGES)
  {
    std::cerr << "SetRepresentationToSurfaceWithEdges() failed." << std::endl;
    return EXIT_FAILURE;
  }

  rep->SetRepresentation(vtkSurfaceRepresentation::WIREFRAME);
  if (rep->GetRepresentation() != vtkSurfaceRepresentation::WIREFRAME)
  {
    std::cerr << "SetRepresentation(int) failed." << std::endl;
    return EXIT_FAILURE;
  }

  // Set surface mode for rendering
  rep->SetRepresentationToSurfaceWithEdges();

  // Test property forwarding
  rep->SetColor(0.2, 0.6, 0.9);
  rep->SetOpacity(0.8);
  rep->SetEdgeColor(0.0, 0.0, 0.0);
  rep->SetAmbient(0.1);
  rep->SetDiffuse(0.7);
  rep->SetSpecular(0.3);
  rep->SetSpecularPower(20.0);
  rep->SetLineWidth(2.0);
  rep->SetPointSize(5.0);

  // Test scalar bar
  rep->SetScalarBarVisibility(true);
  if (!rep->GetScalarBarActor())
  {
    std::cerr << "GetScalarBarActor() returned null after enabling." << std::endl;
    return EXIT_FAILURE;
  }

  // Test the geometry-level representation modes
  rep->SetRepresentationToOutline();
  if (rep->GetRepresentation() != vtkSurfaceRepresentation::OUTLINE)
  {
    std::cerr << "SetRepresentationToOutline() failed." << std::endl;
    return EXIT_FAILURE;
  }

  rep->SetRepresentationToFeatureEdges();
  if (rep->GetRepresentation() != vtkSurfaceRepresentation::FEATURE_EDGES)
  {
    std::cerr << "SetRepresentationToFeatureEdges() failed." << std::endl;
    return EXIT_FAILURE;
  }

  // Going back to a property-level mode must restore surface extraction.
  rep->SetRepresentationToSurfaceWithEdges();

  rep->SetGeneratePointNormals(true);
  if (!rep->GetGeneratePointNormals())
  {
    std::cerr << "SetGeneratePointNormals failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->SetGeneratePointNormals(false);

  rep->SetGenerateCellNormals(true);
  if (!rep->GetGenerateCellNormals())
  {
    std::cerr << "SetGenerateCellNormals failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->SetGenerateCellNormals(false);

  rep->SetFeatureAngle(45.0);
  if (rep->GetFeatureAngle() != 45.0)
  {
    std::cerr << "SetFeatureAngle failed." << std::endl;
    return EXIT_FAILURE;
  }

  rep->SetSplitting(false);
  rep->SetSplitting(true);

  rep->SetTriangulate(true);
  if (!rep->GetTriangulate())
  {
    std::cerr << "SetTriangulate failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->SetTriangulate(false);

  rep->SetNonlinearSubdivisionLevel(2);
  if (rep->GetNonlinearSubdivisionLevel() != 2)
  {
    std::cerr << "SetNonlinearSubdivisionLevel failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->SetNonlinearSubdivisionLevel(1);

  rep->SetMatchBoundariesIgnoringCellOrder(true);
  if (!rep->GetMatchBoundariesIgnoringCellOrder())
  {
    std::cerr << "SetMatchBoundariesIgnoringCellOrder failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->SetMatchBoundariesIgnoringCellOrder(false);

  rep->SetPassThroughCellIds(false);
  if (rep->GetPassThroughCellIds())
  {
    std::cerr << "SetPassThroughCellIds failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->SetPassThroughCellIds(true);

  rep->SetPassThroughPointIds(false);
  if (rep->GetPassThroughPointIds())
  {
    std::cerr << "SetPassThroughPointIds failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->SetPassThroughPointIds(true);

  rep->SetBlockColorsDistinctValues(12);
  if (rep->GetBlockColorsDistinctValues() != 12)
  {
    std::cerr << "SetBlockColorsDistinctValues failed." << std::endl;
    return EXIT_FAILURE;
  }

  rep->SetHideInternalAMRFaces(false);
  if (rep->GetHideInternalAMRFaces())
  {
    std::cerr << "SetHideInternalAMRFaces failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->SetHideInternalAMRFaces(true);

  rep->SetUseNonOverlappingAMRMetaDataForOutlines(false);
  if (rep->GetUseNonOverlappingAMRMetaDataForOutlines())
  {
    std::cerr << "SetUseNonOverlappingAMRMetaDataForOutlines failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->SetUseNonOverlappingAMRMetaDataForOutlines(true);

  rep->SetGenerateProcessIds(true);
  if (!rep->GetGenerateProcessIds())
  {
    std::cerr << "SetGenerateProcessIds failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->SetGenerateProcessIds(false);

  // Test visibility
  rep->SetVisibility(true);
  rep->SetPickable(true);
  rep->SetPosition(0.0, 0.0, 0.0);
  rep->SetOrientation(0.0, 0.0, 0.0);
  rep->SetScale(1.0, 1.0, 1.0);

  // Add to a view and render
  vtkNew<vtkStandardRenderView> view;
  view->GetRenderWindow()->SetOffScreenRendering(true);
  view->SetWindowSize(300, 300);
  view->AddRepresentation(rep);
  view->ResetCamera();
  view->Render();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    view->Start();
  }
  return !retVal;
}
