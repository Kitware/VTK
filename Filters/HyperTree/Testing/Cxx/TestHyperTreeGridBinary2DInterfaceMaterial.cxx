// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .SECTION Thanks
// This test was written by Philippe Pebay,  NexGen Analytics 2017
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestHyperTreeGridBinary2DInterfaceMaterial(int argc, char* argv[])
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  htGrid->SetMaxDepth(6);
  htGrid->SetDimensions(3, 4, 1);     // Dimension 2 in xy plane GridCell 2, 3
  htGrid->SetGridScale(1.5, 1., 10.); // this is to test that orientation fixes scale
  htGrid->SetBranchFactor(2);
  htGrid->SetDescriptor("RRRRR.|.... .R.. RRRR R... R...|.R.. ...R ..RR .R.. R... .... ....|.... "
                        "...R ..R. .... .R.. R...|.... .... .R.. ....|....");
  htGrid->UseMaskOn();
  htGrid->SetDescriptor("RRRRR.|.... .R.. RRRR R... R...|.R.. ...R ..RR .R.. R... .... ....|.... "
                        "...R ..R. .... .R.. R...|.... .... .R.. ....|....");
  htGrid->SetMask("111111|0000 1111 1111 1111 1111|1111 0001 0111 0101 1011 1111 0111|1111 0111 "
                  "1111 1111 1111 1111|1111 1111 1111 1111|1111");
  htGrid->GenerateInterfaceFieldsOn();
  htGrid->Update();
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(htGrid->GetOutput());
  htg->GetCellData()->SetScalars(htg->GetCellData()->GetArray("Depth"));
  vtkHyperTreeGrid* H = htg;
  H->SetHasInterface(true);
  char normalsName[] = "Normals";
  H->SetInterfaceNormalsName(normalsName);
  char interceptsName[] = "Intercepts";
  H->SetInterfaceInterceptsName(interceptsName);

  // Modify intercepts array
  vtkDataArray* interArray =
    vtkHyperTreeGrid::SafeDownCast(htg)->GetCellData()->GetArray("Intercepts");
  for (vtkIdType i = 0; i < interArray->GetNumberOfTuples(); ++i)
  {
    interArray->SetTuple3(i, -.25, -.5, -1.);
  }

  // Geometries
  vtkNew<vtkHyperTreeGridGeometry> geometry1;
  geometry1->SetInputData(H);
  geometry1->Update();
  vtkPolyData* pd = geometry1->GetPolyDataOutput();
  vtkNew<vtkHyperTreeGridGeometry> geometry2;
  geometry2->SetInputData(H);

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(geometry1->GetOutputPort());
  mapper1->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry2->GetOutputPort());
  mapper2->SetScalarRange(pd->GetCellData()->GetArray("Depth")->GetRange());

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetRepresentationToWireframe();
  actor1->GetProperty()->SetColor(.7, .7, .7);
  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);

  // Camera
  double bd[6];
  pd->GetBounds(bd);
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(pd->GetCenter());
  camera->SetPosition(.5 * bd[1], .5 * bd[3], 6.);

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);

  // Render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);
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
