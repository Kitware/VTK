// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Description
 * Test ray intersection of polygons using vtkModifiedBSPTree locator and subsequent extraction of
 * selected cells for a dataset containing ghost arrays
 */

#include "vtkActor.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetMapper.h"
#include "vtkExtractSelection.h"
#include "vtkGlyph3DMapper.h"
#include "vtkLineSource.h"
#include "vtkModifiedBSPTree.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelectionSource.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"

int TestBSPTreeWithGhostArrays(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(301, 300); // Intentional NPOT size

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetRadius(0.05);
  sphereSource->Update();
  vtkSmartPointer<vtkPolyData> sphere = sphereSource->GetOutput();
  sphere->AllocateCellGhostArray();
  vtkSmartPointer<vtkUnsignedCharArray> ghostCells = sphere->GetCellGhostArray();
  ghostCells->SetTuple1(72, vtkDataSetAttributes::HIDDENCELL);
  ghostCells->SetTuple1(19, vtkDataSetAttributes::HIDDENCELL);

  double bounds[6];
  sphere->GetBounds(bounds);
  vtkBoundingBox box(bounds);
  double tol = box.GetDiagonalLength() / 1E6;

  vtkNew<vtkModifiedBSPTree> bspTree;
  bspTree->SetDataSet(sphere);
  bspTree->SetMaxLevel(12);
  bspTree->SetNumberOfCellsPerNode(16);
  bspTree->BuildLocator();

  // Render BSP tree
  vtkNew<vtkPolyData> bspPD;
  bspTree->GenerateRepresentation(2, bspPD);
  vtkNew<vtkPolyDataMapper> bspMapper;
  bspMapper->SetInputData(bspPD);
  vtkNew<vtkActor> bspAc;
  bspAc->SetMapper(bspMapper);
  bspAc->GetProperty()->SetInterpolationToFlat();
  bspAc->GetProperty()->SetOpacity(.3);
  bspAc->GetProperty()->EdgeVisibilityOn();
  bspAc->GetProperty()->SetColor(0.45, 0.25, 0.6);
  renderer->AddActor(bspAc);

  //
  // Intersect Ray with BSP tree full of spheres
  //
  vtkNew<vtkPoints> verts;
  vtkNew<vtkIdList> cellIds;
  double p1[3] = { -0.1, -0.1, -0.1 };
  double p2[3] = { 0.1, 0.1, 0.1 };
  bspTree->IntersectWithLine(p1, p2, tol, verts, cellIds);

  vtkNew<vtkPolyData> intersections;
  vtkNew<vtkCellArray> vertices;
  vtkIdType n = verts->GetNumberOfPoints();
  for (vtkIdType i = 0; i < n; i++)
  {
    vertices->InsertNextCell(1, &i);
  }
  intersections->SetPoints(verts);
  intersections->SetVerts(vertices);
  std::cout << "Number of intersections is " << n << std::endl;

  vtkNew<vtkSelectionSource> selection;
  vtkNew<vtkExtractSelection> extract;
  selection->SetContentType(vtkSelectionNode::INDICES);
  selection->SetFieldType(vtkSelectionNode::CELL);
  for (int i = 0; i < cellIds->GetNumberOfIds(); i++)
  {
    std::cout << cellIds->GetId(i) << ",";
    selection->AddID(-1, cellIds->GetId(i));
  }
  std::cout << std::endl;
  //
  extract->SetInputData(sphere);
  extract->SetSelectionConnection(selection->GetOutputPort());
  extract->Update();

  vtkSmartPointer<vtkUnstructuredGrid> extractedCells =
    vtkUnstructuredGrid::SafeDownCast(extract->GetOutputDataObject(0));
  if (!extractedCells || extractedCells->GetNumberOfCells() != 2)
  {
    return EXIT_FAILURE;
  }

  //
  // Render cloud of target spheres
  //
  vtkNew<vtkPolyDataMapper> smapper;
  smapper->SetInputData(sphere);

  vtkNew<vtkProperty> sproperty;
  sproperty->SetColor(1.0, 1.0, 1.0);
  // sproperty->SetOpacity(0.25);
  sproperty->SetAmbient(0.0);
  sproperty->SetBackfaceCulling(1);
  sproperty->SetFrontfaceCulling(0);
  sproperty->SetRepresentationToPoints();
  // sproperty->SetInterpolationToFlat();

  vtkNew<vtkActor> sactor;
  sactor->SetMapper(smapper);
  sactor->SetProperty(sproperty);
  renderer->AddActor(sactor);

  //
  // Render Intersection points
  //
  vtkNew<vtkGlyph3DMapper> imapper;
  imapper->SetInputData(intersections);
  imapper->SetSourceConnection(sphereSource->GetOutputPort());
  imapper->SetScaleFactor(0.05);

  vtkNew<vtkProperty> iproperty;
  iproperty->SetOpacity(1.0);
  iproperty->SetColor(0.0, 0.0, 1.0);
  iproperty->SetBackfaceCulling(1);
  iproperty->SetFrontfaceCulling(0);

  vtkNew<vtkActor> iactor;
  iactor->SetMapper(imapper);
  iactor->SetProperty(iproperty);
  renderer->AddActor(iactor);

  //
  // Render Ray
  //
  vtkNew<vtkLineSource> ray;
  ray->SetPoint1(p1);
  ray->SetPoint2(p2);

  vtkNew<vtkPolyDataMapper> rmapper;
  rmapper->SetInputConnection(ray->GetOutputPort(0));

  vtkNew<vtkActor> lactor;
  lactor->SetMapper(rmapper);
  renderer->AddActor(lactor);

  //
  // Render Intersected Cells (extracted using selection)
  //
  vtkNew<vtkDataSetMapper> cmapper;
  cmapper->SetInputConnection(extract->GetOutputPort(0));

  vtkNew<vtkProperty> cproperty;
  cproperty->SetColor(0.0, 1.0, 1.0);
  cproperty->SetBackfaceCulling(0);
  cproperty->SetFrontfaceCulling(0);
  cproperty->SetAmbient(1.0);
  cproperty->SetLineWidth(3.0);
  cproperty->SetRepresentationToWireframe();
  cproperty->SetInterpolationToFlat();

  vtkNew<vtkActor> cactor;
  cactor->SetMapper(cmapper);
  cactor->SetProperty(cproperty);
  renderer->AddActor(cactor);

  //
  // Standard testing code.
  //
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);
  renWin->Render();
  renderer->GetActiveCamera()->SetPosition(0.0, 0.15, 0.0);
  renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.0, 0.0);
  renderer->GetActiveCamera()->SetViewUp(0.0, 0.0, 1.0);
  renderer->SetBackground(0.0, 0.0, 0.0);
  renderer->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
