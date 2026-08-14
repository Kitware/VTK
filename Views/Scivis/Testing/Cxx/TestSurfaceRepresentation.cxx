// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <cstring>
#include <iostream>

#include "vtkActor.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkGeometryFilterDispatcher.h"
#include "vtkLookupTable.h"
#include "vtkMapper.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScivisView.h"
#include "vtkSphereSource.h"
#include "vtkSurfaceRepresentation.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

namespace
{

// Coloring by a field data array.  Uses its own representation so that the
// rendered state below is untouched.
int TestFieldArrayColoring()
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSurfaceRepresentation> rep;
  rep->SetInputConnection(sphere->GetOutputPort());

  rep->ColorByFieldArray("BlockId");
  vtkMapper* mapper = rep->GetActor()->GetMapper();
  if (mapper->GetScalarMode() != VTK_SCALAR_MODE_USE_FIELD_DATA)
  {
    std::cerr << "ColorByFieldArray did not select field data scalars." << std::endl;
    return EXIT_FAILURE;
  }
  if (!mapper->GetArrayName() || strcmp(mapper->GetArrayName(), "BlockId") != 0)
  {
    std::cerr << "ColorByFieldArray did not select the named array." << std::endl;
    return EXIT_FAILURE;
  }
  if (!mapper->GetScalarVisibility())
  {
    std::cerr << "ColorByFieldArray left scalar coloring off." << std::endl;
    return EXIT_FAILURE;
  }

  // The default consumes the array one tuple per cell.
  if (rep->GetFieldDataTupleId() != -1)
  {
    std::cerr << "The field data tuple id should start at -1." << std::endl;
    return EXIT_FAILURE;
  }

  // A tuple id colors the whole surface with that one tuple.
  rep->SetFieldDataTupleId(2);
  if (rep->GetFieldDataTupleId() != 2 || mapper->GetFieldDataTupleId() != 2)
  {
    std::cerr << "SetFieldDataTupleId did not reach the mapper." << std::endl;
    return EXIT_FAILURE;
  }

  // Switching to a point array leaves field data mode behind.
  rep->ColorByPointArray("Normals");
  if (mapper->GetScalarMode() != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
  {
    std::cerr << "ColorByPointArray did not replace the field data mode." << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

// The range belongs to the lookup table, so rendering must not write over it.
int TestLookupTableRangeSurvivesRendering()
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSurfaceRepresentation> rep;
  rep->SetInputConnection(sphere->GetOutputPort());
  rep->ColorByPointArray("Normals");

  vtkNew<vtkLookupTable> lut;
  lut->SetRange(30.0, 280.0);
  rep->SetColorMap(lut);

  vtkNew<vtkScivisView> view;
  view->GetRenderWindow()->SetOffScreenRendering(true);
  view->SetWindowSize(120, 120);
  view->AddRepresentation(rep);
  view->ResetCamera();
  view->Render();

  double* range = lut->GetRange();
  if (range[0] != 30.0 || range[1] != 280.0)
  {
    std::cerr << "Rendering overwrote the lookup table range with [" << range[0] << ", " << range[1]
              << "]." << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

}

int TestSurfaceRepresentation(int argc, char* argv[])
{
  if (TestFieldArrayColoring() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  if (TestLookupTableRangeSurvivesRendering() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }

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
  rep->GetProperty()->SetAmbient(0.1);
  rep->GetProperty()->SetDiffuse(0.7);
  rep->GetProperty()->SetSpecular(0.3);
  rep->GetProperty()->SetSpecularPower(20.0);
  rep->GetProperty()->SetLineWidth(2.0);
  rep->GetProperty()->SetPointSize(5.0);

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

  rep->GetGeometryFilter()->SetGeneratePointNormals(true);
  if (!rep->GetGeometryFilter()->GetGeneratePointNormals())
  {
    std::cerr << "SetGeneratePointNormals failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->GetGeometryFilter()->SetGeneratePointNormals(false);

  rep->GetGeometryFilter()->SetGenerateCellNormals(true);
  if (!rep->GetGeometryFilter()->GetGenerateCellNormals())
  {
    std::cerr << "SetGenerateCellNormals failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->GetGeometryFilter()->SetGenerateCellNormals(false);

  rep->GetGeometryFilter()->SetFeatureAngle(45.0);
  if (rep->GetGeometryFilter()->GetFeatureAngle() != 45.0)
  {
    std::cerr << "SetFeatureAngle failed." << std::endl;
    return EXIT_FAILURE;
  }

  rep->GetGeometryFilter()->SetSplitting(false);
  rep->GetGeometryFilter()->SetSplitting(true);

  rep->GetGeometryFilter()->SetTriangulate(true);
  if (!rep->GetGeometryFilter()->GetTriangulate())
  {
    std::cerr << "SetTriangulate failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->GetGeometryFilter()->SetTriangulate(false);

  rep->GetGeometryFilter()->SetNonlinearSubdivisionLevel(2);
  if (rep->GetGeometryFilter()->GetNonlinearSubdivisionLevel() != 2)
  {
    std::cerr << "SetNonlinearSubdivisionLevel failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->GetGeometryFilter()->SetNonlinearSubdivisionLevel(1);

  rep->GetGeometryFilter()->SetMatchBoundariesIgnoringCellOrder(true);
  if (!rep->GetGeometryFilter()->GetMatchBoundariesIgnoringCellOrder())
  {
    std::cerr << "SetMatchBoundariesIgnoringCellOrder failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->GetGeometryFilter()->SetMatchBoundariesIgnoringCellOrder(false);

  rep->GetGeometryFilter()->SetPassThroughCellIds(false);
  if (rep->GetGeometryFilter()->GetPassThroughCellIds())
  {
    std::cerr << "SetPassThroughCellIds failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->GetGeometryFilter()->SetPassThroughCellIds(true);

  rep->GetGeometryFilter()->SetPassThroughPointIds(false);
  if (rep->GetGeometryFilter()->GetPassThroughPointIds())
  {
    std::cerr << "SetPassThroughPointIds failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->GetGeometryFilter()->SetPassThroughPointIds(true);

  rep->GetGeometryFilter()->SetBlockColorsDistinctValues(12);
  if (rep->GetGeometryFilter()->GetBlockColorsDistinctValues() != 12)
  {
    std::cerr << "SetBlockColorsDistinctValues failed." << std::endl;
    return EXIT_FAILURE;
  }

  rep->GetGeometryFilter()->SetHideInternalAMRFaces(false);
  if (rep->GetGeometryFilter()->GetHideInternalAMRFaces())
  {
    std::cerr << "SetHideInternalAMRFaces failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->GetGeometryFilter()->SetHideInternalAMRFaces(true);

  rep->GetGeometryFilter()->SetUseNonOverlappingAMRMetaDataForOutlines(false);
  if (rep->GetGeometryFilter()->GetUseNonOverlappingAMRMetaDataForOutlines())
  {
    std::cerr << "SetUseNonOverlappingAMRMetaDataForOutlines failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->GetGeometryFilter()->SetUseNonOverlappingAMRMetaDataForOutlines(true);

  rep->GetGeometryFilter()->SetGenerateProcessIds(true);
  if (!rep->GetGeometryFilter()->GetGenerateProcessIds())
  {
    std::cerr << "SetGenerateProcessIds failed." << std::endl;
    return EXIT_FAILURE;
  }
  rep->GetGeometryFilter()->SetGenerateProcessIds(false);

  // Test visibility
  rep->SetVisibility(true);
  rep->GetActor()->SetPickable(true);
  rep->GetActor()->SetPosition(0.0, 0.0, 0.0);
  rep->GetActor()->SetOrientation(0.0, 0.0, 0.0);
  rep->GetActor()->SetScale(1.0, 1.0, 1.0);

  // Add to a view and render
  vtkNew<vtkScivisView> view;
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
