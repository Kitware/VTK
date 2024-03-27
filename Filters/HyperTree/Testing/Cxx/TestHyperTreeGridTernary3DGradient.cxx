// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .SECTION Thanks
// This test was written by Charles Gueunet, 2022
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkArrowSource.h"
#include "vtkBitArray.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkExtractTensorComponents.h"
#include "vtkGlyph3D.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridCellCenters.h"
#include "vtkHyperTreeGridGradient.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLHyperTreeGridReader.h"

int TestHyperTreeGridTernary3DGradient(int argc, char* argv[])
{

  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMR/htg3d.htg");
  std::string fileName(fileNameC);
  delete[] fileNameC;

  vtkNew<vtkXMLHyperTreeGridReader> htGrid;
  htGrid->SetFileName(fileName.c_str());
  htGrid->Update();
  vtkHyperTreeGrid* ht = vtkHyperTreeGrid::SafeDownCast(htGrid->GetOutputDataObject(0));
  vtkNew<vtkBitArray> emptyMask;
  ht->SetMask(emptyMask);

  // Add vector attributes
  vtkDataArray* depth = ht->GetCellData()->GetArray("Depth");
  vtkIdType nbCells = ht->GetNumberOfCells();
  vtkNew<vtkDoubleArray> vectArr;
  vectArr->SetNumberOfComponents(3);
  vectArr->SetNumberOfTuples(nbCells);
  vectArr->SetName("Vect");
  for (vtkIdType cellId = 0; cellId < nbCells; cellId++)
  {
    double cellDepth = depth->GetTuple1(cellId);
    vectArr->SetTuple3(cellId, cellDepth, cellId, cellId * cellDepth);
  }
  ht->GetCellData()->AddArray(vectArr);
  ht->GetCellData()->SetActiveVectors("Vect");

  // Gradient
  vtkNew<vtkHyperTreeGridGradient> gradient;
  gradient->SetInputConnection(htGrid->GetOutputPort());
  gradient->SetMode(vtkHyperTreeGridGradient::UNLIMITED);
  gradient->SetInputArrayToProcess(0, 0, 0, vtkDataSet::CELL, "Vect");
  gradient->ComputeDivergenceOn();
  gradient->ComputeVorticityOn();

  // extract cell centers
  vtkNew<vtkHyperTreeGridCellCenters> centers;
  centers->SetInputConnection(gradient->GetOutputPort());
  centers->SetVertexCells(true);

  vtkNew<vtkExtractTensorComponents> extractVect;
  extractVect->SetInputConnection(centers->GetOutputPort());
  extractVect->SetInputArrayToProcess(0, 0, 0, vtkDataSet::POINT, "Gradient");
  extractVect->ExtractVectorsOn();

  // Generate glyphs
  vtkNew<vtkArrowSource> glyph;
  vtkNew<vtkGlyph3D> glypher;
  glypher->SetInputConnection(extractVect->GetOutputPort());
  glypher->SetSourceConnection(glyph->GetOutputPort());
  glypher->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "TensorVectors");
  glypher->OrientOn();
  glypher->SetVectorModeToUseVector();
  glypher->ScalingOn();
  glypher->SetScaleModeToScaleByVector();
  glypher->SetScaleFactor(10);
  glypher->Update();
  glypher->GetOutput(0)->GetPointData()->SetActiveScalars("Divergence");

  // mapper
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(glypher->GetOutputPort());
  mapper1->SetColorModeToDefault();
  mapper1->SetScalarVisibility(true);
  mapper1->SetScalarRange(-11, 34);
  // color by magnitude
  vtkNew<vtkLookupTable> colormap;
  colormap->SetVectorModeToMagnitude();
  colormap->Build();
  mapper1->SetLookupTable(colormap);

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0., 0., 0.);
  renderer->AddActor(actor1);

  // Render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(600, 600);
  renWin->SetMultiSamples(0);

  // Interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Render and test
  renWin->Render();

  int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
