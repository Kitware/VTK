// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test demonstrates the vtkCellCentersPointPlacer. The placer may
// be used to constrain handle widgets to the centers of cells. Thus it
// may be used by any of the widgets that use the handles (distance, angle
// etc).
//   Here we demonstrate constraining the distance widget to the centers
// of various cells.

#include "vtkActor.h"
#include "vtkAxisActor2D.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellCentersPointPlacer.h"
#include "vtkCellPicker.h"
#include "vtkDataSetMapper.h"
#include "vtkDistanceRepresentation2D.h"
#include "vtkDistanceWidget.h"
#include "vtkHexahedron.h"
#include "vtkMatrix4x4.h"
#include "vtkMatrixToLinearTransform.h"
#include "vtkPentagonalPrism.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkPoints.h"
#include "vtkPolyhedron.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkPyramid.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTesting.h"
#include "vtkTetra.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

#include <vector>

//------------------------------------------------------------------------------
constexpr char TestCellCentersPointPlacerEventLog[] = "# StreamVersion 1\n"
                                                      "EnterEvent 384 226 0 0 0 0 0\n"

                                                      "MouseMoveEvent 197 106 0 0 0 0 0\n"
                                                      "LeftButtonPressEvent 197 106 0 0 0 0 0\n"
                                                      "LeftButtonReleaseEvent 197 106 0 0 0 0 0\n"

                                                      "MouseMoveEvent 362 194 0 0 0 0 0\n"
                                                      "LeftButtonPressEvent 362 194 0 0 0 0 0\n"
                                                      "LeftButtonReleaseEvent 362 194 0 0 0 0 0\n"

                                                      "MouseMoveEvent 362 194 0 0 0 0 0\n"
                                                      "LeftButtonPressEvent 202 110 0 0 0 0 0\n"
                                                      "MouseMoveEvent 204 235 0 0 0 0 0\n"
                                                      "LeftButtonReleaseEvent 204 235 0 0 0 0 0\n"

                                                      "MouseMoveEvent 363 192 0 0 0 0 0\n"
                                                      "LeftButtonPressEvent 363 192 0 0 0 0 0\n"
                                                      "MouseMoveEvent 491 315 0 0 0 0 0\n"
                                                      "LeftButtonReleaseEvent 491 315 0 0 0 0 0\n"

                                                      "MouseMoveEvent 213 241 0 0 0 0 0\n"
                                                      "LeftButtonPressEvent 213 241 0 0 0 0 0\n"
                                                      "MouseMoveEvent 478 159 0 0 0 0 0\n"
                                                      "LeftButtonReleaseEvent 478 159 0 0 0 0 0\n"

                                                      "KeyPressEvent 369 155 0 0 113 1 q\n"
                                                      "CharEvent 369 155 0 0 113 1 q\n"

                                                      "ExitEvent 369 155 0 0 113 1 q\n";

//------------------------------------------------------------------------------
// Create cells of various types
void CreateHexahedronActor(vtkActor* actor);
void CreatePentagonalPrismActor(vtkActor* actor);
void CreatePyramidActor(vtkActor* actor);
void CreateTetraActor(vtkActor* actor);
void CreateVoxelActor(vtkActor* actor);
void CreateWedgeActor(vtkActor* actor);

//------------------------------------------------------------------------------
int TestCellCentersPointPlacer(int argc, char* argv[])
{
  std::vector<vtkSmartPointer<vtkActor>> actors;

  vtkSmartPointer<vtkActor> hexahedronActor = vtkSmartPointer<vtkActor>::New();
  CreateHexahedronActor(hexahedronActor);
  actors.push_back(hexahedronActor);

  vtkSmartPointer<vtkActor> pentagonalPrismActor = vtkSmartPointer<vtkActor>::New();
  CreatePentagonalPrismActor(pentagonalPrismActor);
  actors.push_back(pentagonalPrismActor);

  vtkSmartPointer<vtkActor> pyramidActor = vtkSmartPointer<vtkActor>::New();
  CreatePyramidActor(pyramidActor);
  actors.push_back(pyramidActor);

  vtkSmartPointer<vtkActor> tetraActor = vtkSmartPointer<vtkActor>::New();
  CreateTetraActor(tetraActor);
  actors.push_back(tetraActor);

  vtkSmartPointer<vtkActor> voxelActor = vtkSmartPointer<vtkActor>::New();
  CreateVoxelActor(voxelActor);
  actors.push_back(voxelActor);

  vtkSmartPointer<vtkActor> wedgeActor = vtkSmartPointer<vtkActor>::New();
  CreateWedgeActor(wedgeActor);
  actors.push_back(wedgeActor);

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();

  int gridDimensions = 3;
  int rendererSize = 200;

  // Create a render window, renderer and render window interactor.
  // Add the cells to the renderer, in a grid layout. We accomplish
  // this by using a transform filter to translate and arrange on
  // a grid.

  vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->SetSize(rendererSize * gridDimensions, rendererSize * gridDimensions);
  renderWindow->AddRenderer(renderer);
  renderer->SetBackground(.2, .3, .4);

  // Create a point placer to constrain to the cell centers and add
  // each of the actors to the placer, so that it includes them in
  // its constraints.
  vtkSmartPointer<vtkCellCentersPointPlacer> pointPlacer =
    vtkSmartPointer<vtkCellCentersPointPlacer>::New();

  for (int row = 0; row < gridDimensions; row++)
  {
    for (int col = 0; col < gridDimensions; col++)
    {
      int index = row * gridDimensions + col;

      if (index > static_cast<int>(actors.size() - 1))
      {
        continue;
      }

      vtkSmartPointer<vtkTransformFilter> transformFilter =
        vtkSmartPointer<vtkTransformFilter>::New();
      vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
      matrix->SetElement(0, 3, 5 * col);
      matrix->SetElement(1, 3, 5 * row);
      vtkSmartPointer<vtkMatrixToLinearTransform> mlt =
        vtkSmartPointer<vtkMatrixToLinearTransform>::New();
      mlt->SetInput(matrix);
      transformFilter->SetInputConnection(actors[index]->GetMapper()->GetInputConnection(0, 0));
      transformFilter->SetTransform(mlt);
      transformFilter->Update();

      vtkSmartPointer<vtkDataSetMapper> mapper2 = vtkSmartPointer<vtkDataSetMapper>::New();
      mapper2->SetInputConnection(transformFilter->GetOutputPort());
      actors[index]->SetMapper(mapper2);

      renderer->AddActor(actors[index]);
      pointPlacer->AddProp(actors[index]);
    }
  }

  // Default colors
  actors[0]->GetProperty()->SetColor(1, 0, 0.5);
  actors[1]->GetProperty()->SetColor(0, 1, 0);
  actors[2]->GetProperty()->SetColor(0, 0, 1);
  actors[3]->GetProperty()->SetColor(1, 1, 0);
  actors[4]->GetProperty()->SetColor(1, 0, 1);
  actors[5]->GetProperty()->SetColor(0, 1, 1);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(30);
  renderer->GetActiveCamera()->Elevation(-30);
  renderer->ResetCameraClippingRange();

  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderWindow->Render();

  // Now add a distance widget.

  vtkSmartPointer<vtkDistanceWidget> widget = vtkSmartPointer<vtkDistanceWidget>::New();
  widget->CreateDefaultRepresentation();
  vtkSmartPointer<vtkDistanceRepresentation2D> rep =
    vtkSmartPointer<vtkDistanceRepresentation2D>::New();
  rep->GetAxis()->GetProperty()->SetColor(1.0, 0.0, 0.0);

  // Create a 3D handle representation template for this distance
  // widget
  vtkSmartPointer<vtkPointHandleRepresentation3D> handleRep3D =
    vtkSmartPointer<vtkPointHandleRepresentation3D>::New();
  handleRep3D->GetProperty()->SetLineWidth(4.0);
  rep->SetHandleRepresentation(handleRep3D);
  handleRep3D->GetProperty()->SetColor(0.8, 0.2, 0);
  widget->SetRepresentation(rep);

  // Instantiate the handles and have them be constrained by the
  // placer.
  rep->InstantiateHandleRepresentation();
  rep->GetPoint1Representation()->SetPointPlacer(pointPlacer);
  rep->GetPoint2Representation()->SetPointPlacer(pointPlacer);

  // With a "snap" constraint, we can't have a smooth motion anymore, so
  // turn it off.
  static_cast<vtkPointHandleRepresentation3D*>(rep->GetPoint1Representation())->SmoothMotionOff();
  static_cast<vtkPointHandleRepresentation3D*>(rep->GetPoint2Representation())->SmoothMotionOff();

  widget->SetInteractor(renderWindowInteractor);
  widget->SetEnabled(1);

  renderWindow->Render();

  return vtkTesting::InteractorEventLoop(
    argc, argv, renderWindowInteractor, TestCellCentersPointPlacerEventLog);
}

//------------------------------------------------------------------------------
void CreateHexahedronActor(vtkActor* actor)
{
  // Setup the coordinates of eight points
  // (the two faces must be in counter clockwise order as viewd from the outside)

  // Create the points
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 1.0, 0.0);
  points->InsertNextPoint(0.0, 1.0, 0.0);
  points->InsertNextPoint(0.0, 0.0, 1.0);
  points->InsertNextPoint(1.0, 0.0, 1.0);
  points->InsertNextPoint(1.0, 1.0, 1.0);
  points->InsertNextPoint(0.0, 1.0, 1.0);

  // Create a hexahedron from the points
  vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();
  hex->GetPointIds()->SetId(0, 0);
  hex->GetPointIds()->SetId(1, 1);
  hex->GetPointIds()->SetId(2, 2);
  hex->GetPointIds()->SetId(3, 3);
  hex->GetPointIds()->SetId(4, 4);
  hex->GetPointIds()->SetId(5, 5);
  hex->GetPointIds()->SetId(6, 6);
  hex->GetPointIds()->SetId(7, 7);

  // Add the hexahedron to a cell array
  vtkSmartPointer<vtkCellArray> hexs = vtkSmartPointer<vtkCellArray>::New();
  hexs->InsertNextCell(hex);

  // Add the points and hexahedron to an unstructured grid
  vtkSmartPointer<vtkUnstructuredGrid> uGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  uGrid->SetPoints(points);
  uGrid->InsertNextCell(hex->GetCellType(), hex->GetPointIds());

  // Visualize
  vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(uGrid);

  actor->SetMapper(mapper);
}

//------------------------------------------------------------------------------
void CreatePentagonalPrismActor(vtkActor* actor)
{
  // Create the points
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(3, 0, 0);
  points->InsertNextPoint(4, 2, 0);
  points->InsertNextPoint(2, 4, 0);
  points->InsertNextPoint(0, 2, 0);
  points->InsertNextPoint(1, 0, 4);
  points->InsertNextPoint(3, 0, 4);
  points->InsertNextPoint(4, 2, 4);
  points->InsertNextPoint(2, 4, 4);
  points->InsertNextPoint(0, 2, 4);

  // Pentagonal Prism
  vtkSmartPointer<vtkPentagonalPrism> pentagonalPrism = vtkSmartPointer<vtkPentagonalPrism>::New();
  pentagonalPrism->GetPointIds()->SetId(0, 0);
  pentagonalPrism->GetPointIds()->SetId(1, 1);
  pentagonalPrism->GetPointIds()->SetId(2, 2);
  pentagonalPrism->GetPointIds()->SetId(3, 3);
  pentagonalPrism->GetPointIds()->SetId(4, 4);
  pentagonalPrism->GetPointIds()->SetId(5, 5);
  pentagonalPrism->GetPointIds()->SetId(6, 6);
  pentagonalPrism->GetPointIds()->SetId(7, 7);
  pentagonalPrism->GetPointIds()->SetId(8, 8);
  pentagonalPrism->GetPointIds()->SetId(9, 9);

  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();
  cellArray->InsertNextCell(pentagonalPrism);

  // Add the points and hexahedron to an unstructured grid
  vtkSmartPointer<vtkUnstructuredGrid> uGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  uGrid->SetPoints(points);
  uGrid->InsertNextCell(pentagonalPrism->GetCellType(), pentagonalPrism->GetPointIds());

  // Visualize
  vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(uGrid);

  actor->SetMapper(mapper);
}

//------------------------------------------------------------------------------
void CreatePyramidActor(vtkActor* actor)
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

  float p0[3] = { 1.0, 1.0, 1.0 };
  float p1[3] = { -1.0, 1.0, 1.0 };
  float p2[3] = { -1.0, -1.0, 1.0 };
  float p3[3] = { 1.0, -1.0, 1.0 };
  float p4[3] = { 0.0, 0.0, 0.0 };

  points->InsertNextPoint(p0);
  points->InsertNextPoint(p1);
  points->InsertNextPoint(p2);
  points->InsertNextPoint(p3);
  points->InsertNextPoint(p4);

  vtkSmartPointer<vtkPyramid> pyramid = vtkSmartPointer<vtkPyramid>::New();
  pyramid->GetPointIds()->SetId(0, 0);
  pyramid->GetPointIds()->SetId(1, 1);
  pyramid->GetPointIds()->SetId(2, 2);
  pyramid->GetPointIds()->SetId(3, 3);
  pyramid->GetPointIds()->SetId(4, 4);

  vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
  cells->InsertNextCell(pyramid);

  vtkSmartPointer<vtkUnstructuredGrid> ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
  ug->SetPoints(points);
  ug->InsertNextCell(pyramid->GetCellType(), pyramid->GetPointIds());

  // Create an actor and mapper
  vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(ug);

  actor->SetMapper(mapper);
}

//------------------------------------------------------------------------------
void CreateTetraActor(vtkActor* actor)
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(0, 1, 1);
  points->InsertNextPoint(5, 5, 5);
  points->InsertNextPoint(6, 5, 5);
  points->InsertNextPoint(6, 6, 5);
  points->InsertNextPoint(5, 6, 6);

  vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(points);

  vtkSmartPointer<vtkTetra> tetra = vtkSmartPointer<vtkTetra>::New();
  tetra->GetPointIds()->SetId(0, 0);
  tetra->GetPointIds()->SetId(1, 1);
  tetra->GetPointIds()->SetId(2, 2);
  tetra->GetPointIds()->SetId(3, 3);

  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();
  cellArray->InsertNextCell(tetra);
  unstructuredGrid->SetCells(VTK_TETRA, cellArray);

  // Create a mapper and actor
  vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(unstructuredGrid);

  actor->SetMapper(mapper);
}

//------------------------------------------------------------------------------
void CreateVoxelActor(vtkActor* actor)
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(0, 0, 1);
  points->InsertNextPoint(1, 0, 1);
  points->InsertNextPoint(0, 1, 1);
  points->InsertNextPoint(1, 1, 1);

  vtkSmartPointer<vtkVoxel> voxel = vtkSmartPointer<vtkVoxel>::New();
  voxel->GetPointIds()->SetId(0, 0);
  voxel->GetPointIds()->SetId(1, 1);
  voxel->GetPointIds()->SetId(2, 2);
  voxel->GetPointIds()->SetId(3, 3);
  voxel->GetPointIds()->SetId(4, 4);
  voxel->GetPointIds()->SetId(5, 5);
  voxel->GetPointIds()->SetId(6, 6);
  voxel->GetPointIds()->SetId(7, 7);

  vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
  cells->InsertNextCell(voxel);

  vtkSmartPointer<vtkUnstructuredGrid> ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
  ug->SetPoints(points);
  ug->InsertNextCell(voxel->GetCellType(), voxel->GetPointIds());

  // Visualize
  vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(ug);

  actor->SetMapper(mapper);
}

//------------------------------------------------------------------------------
void CreateWedgeActor(vtkActor* actor)
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(0, .5, .5);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(1, 0.0, 0.0);
  points->InsertNextPoint(1, .5, .5);

  vtkSmartPointer<vtkWedge> wedge = vtkSmartPointer<vtkWedge>::New();
  wedge->GetPointIds()->SetId(0, 0);
  wedge->GetPointIds()->SetId(1, 1);
  wedge->GetPointIds()->SetId(2, 2);
  wedge->GetPointIds()->SetId(3, 3);
  wedge->GetPointIds()->SetId(4, 4);
  wedge->GetPointIds()->SetId(5, 5);

  vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
  cells->InsertNextCell(wedge);

  vtkSmartPointer<vtkUnstructuredGrid> ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
  ug->SetPoints(points);
  ug->InsertNextCell(wedge->GetCellType(), wedge->GetPointIds());

  // Visualize
  vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(ug);

  actor->SetMapper(mapper);
}
