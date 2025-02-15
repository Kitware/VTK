// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera.h"
#include "vtkDataSetMapper.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridAxisCut.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkNew.h"
#include "vtkOutlineSource.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLHyperTreeGridReader.h"

#include <limits>

/**
 * Test the behavior of vtkHyperTreeGridAxisCut when the cutting plane is coincident with some faces
 * of the HTG cells geometry. In such cases, the plan should be considered as "inside" if it is
 * coincident with the opposite faces of the cell origin.
 */
int TestHyperTreeGrid3DAxisCutCoincidentPlane(int argc, char* argv[])
{
  // Read HTG test data
  std::string filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/htg_for_axis_aligned_cut.htg");
  vtkNew<vtkXMLHyperTreeGridReader> htgReader;
  htgReader->SetFileName(filename.c_str());
  htgReader->Update();

  // Compute HTG bounds
  vtkHyperTreeGrid* htg = htgReader->GetOutput();
  if (!htg)
  {
    cerr << "Unable to read input HTG (" + filename + ")";
    return EXIT_FAILURE;
  }
  std::array<double, 6> bounds;
  htg->GetBounds(bounds.data());

  // Define test plane positions
  std::array<double, 4> planePositions = {
    0.014799999999999994,                                     // Coincident, inside the HTG
    bounds[1] + std::numeric_limits<double>::epsilon() * 0.5, // Coincident, htg boundary
    bounds[1] + std::numeric_limits<double>::epsilon(),       // Outside
    bounds[0],                                                // Outside
  };

  std::array<double, 4> expectedNbOfCutCells = { 110, 11, 0, 0 };
  std::array<vtkSmartPointer<vtkHyperTreeGrid>, 4> cuts;

  vtkNew<vtkHyperTreeGridAxisCut> cutter;
  cutter->SetInputData(htg);
  cutter->SetPlaneNormalAxis(0); // X

  // Test cut results
  for (unsigned int i = 0; i < 4; i++)
  {
    cutter->SetPlanePosition(planePositions[i]);
    cutter->Update();

    vtkHyperTreeGrid* cut = cutter->GetHyperTreeGridOutput();
    if (!cut)
    {
      std::cerr << "Unable to retrieve the HTG cut " << i << "." << endl;
      return EXIT_FAILURE;
    }
    if (cut->GetNumberOfCells() != expectedNbOfCutCells[i])
    {
      cerr << "Wrong number of cells in the HTG slice. Expected 110, got "
           << cut->GetNumberOfCells() << endl;
      return EXIT_FAILURE;
    }

    // Store cut
    cuts[i] = vtkSmartPointer<vtkHyperTreeGrid>::New();
    cuts[i]->DeepCopy(cut);
  }

  // Geometry
  vtkNew<vtkOutlineSource> htgOutline;
  htgOutline->SetBounds(bounds.data());
  vtkNew<vtkHyperTreeGridGeometry> geometryCutIn;
  geometryCutIn->SetInputData(cuts[0]);
  vtkNew<vtkHyperTreeGridGeometry> geometryCutOut;
  geometryCutOut->SetInputData(cuts[1]);

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkDataSetMapper> mapperCutIn;
  mapperCutIn->SetInputConnection(geometryCutIn->GetOutputPort());
  vtkNew<vtkDataSetMapper> mapperCutBound;
  mapperCutBound->SetInputConnection(geometryCutOut->GetOutputPort());
  vtkNew<vtkDataSetMapper> mapperHTG;
  mapperHTG->SetInputConnection(htgOutline->GetOutputPort());

  // Actors
  vtkNew<vtkActor> actorCutIn;
  actorCutIn->SetMapper(mapperCutIn);
  actorCutIn->GetProperty()->SetRepresentationToSurface();
  actorCutIn->GetProperty()->SetEdgeVisibility(true);
  vtkNew<vtkActor> actorCutBound;
  actorCutBound->SetMapper(mapperCutBound);
  actorCutBound->GetProperty()->SetRepresentationToSurface();
  actorCutBound->GetProperty()->SetEdgeVisibility(true);
  vtkNew<vtkActor> actorHTG;
  actorHTG->SetMapper(mapperHTG);

  // Camera
  vtkNew<vtkCamera> camera;
  camera->SetPosition(.5, .5, 0.);

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->AddActor(actorCutIn);
  renderer->AddActor(actorCutBound);
  renderer->AddActor(actorHTG);
  renderer->ResetCamera();

  // Render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);
  renWin->SetMultiSamples(0);

  // Interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Render and test against baseline
  renWin->Render();
  int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  else if (retVal == vtkRegressionTester::FAILED)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
