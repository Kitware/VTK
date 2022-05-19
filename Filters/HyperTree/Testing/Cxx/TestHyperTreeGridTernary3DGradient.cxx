/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary3DContour.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Charles Gueunet, 2022
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkArrowSource.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
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
  ht->GetCellData()->SetActiveScalars("Depth");

  // Gradient
  vtkNew<vtkHyperTreeGridGradient> gradient;
  gradient->SetInputConnection(htGrid->GetOutputPort());

  // extract cell centers
  vtkNew<vtkHyperTreeGridCellCenters> centers;
  centers->SetInputConnection(gradient->GetOutputPort());
  centers->SetVertexCells(true);

  // Generate glyphs
  vtkNew<vtkArrowSource> glyph;
  vtkNew<vtkGlyph3D> glypher;
  glypher->SetInputConnection(centers->GetOutputPort());
  glypher->SetSourceConnection(glyph->GetOutputPort());
  glypher->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Gradient");
  glypher->OrientOn();
  glypher->SetVectorModeToUseVector();
  glypher->ScalingOn();
  glypher->SetScaleModeToScaleByVector();
  glypher->SetScaleFactor(0.3);

  // mapper
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(glypher->GetOutputPort());
  mapper1->SetColorModeToDefault();
  mapper1->SetScalarVisibility(true);
  mapper1->SetScalarRange(10, 50);
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

  int retVal = vtkRegressionTestImageThreshold(renWin, 60);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
