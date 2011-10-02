/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOpenGLPolyDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of vtkLabelPlacementMapper
// .SECTION Description
// this program tests vtkLabelPlacementMapper which uses a sophisticated algorithm to
// prune labels/icons preventing them from overlapping.

#include <vtkCellArray.h>
#include <vtkProperty2D.h>
#include <vtkPolyData.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkActor2D.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTestUtilities.h>
#include <vtkRegressionTestImage.h>

int TestTransformCoordinateUseDouble(int argc, char *argv[])
{
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);

  // Create a box around the renderers
  vtkNew<vtkPolyData> poly;
  vtkNew<vtkPoints> points;

  // Shift the points so they don't fall right between 2 pixels but on the on
  // located on the top right.
  double shift = 0.0002;
  points->InsertNextPoint(0. + shift, 0. + shift, 0); // bottom-left
  points->InsertNextPoint(1. + shift, 0. + shift, 0); // bottom-right
  points->InsertNextPoint(1. + shift, 1. + shift, 0); // top-right
  points->InsertNextPoint(0. + shift, 1. + shift, 0); // top-left

  vtkNew<vtkCellArray> cells;
  cells->InsertNextCell(5);
  cells->InsertCellPoint(0);
  cells->InsertCellPoint(1);
  cells->InsertCellPoint(2);
  cells->InsertCellPoint(3);
  cells->InsertCellPoint(0);

  poly->SetPoints(points.GetPointer());
  poly->SetLines(cells.GetPointer());

  int i = 6;
  double x = 0.;
  double y = 1. / 8.;
  double width = 1. / 4.;
  double height = 1. / 8.;

  vtkNew<vtkRenderer> emptyRenderer;
  emptyRenderer->SetViewport(0 , 0 , width, height);
  renderWindow->AddRenderer(emptyRenderer.GetPointer());

  while (--i)
    {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(x , y , x + width, y + height);

    vtkNew<vtkCoordinate> boxCoordinate;
    boxCoordinate->SetCoordinateSystemToNormalizedViewport();
    boxCoordinate->SetViewport(renderer.GetPointer());

    vtkNew<vtkPolyDataMapper2D> polyDataMapper;
    polyDataMapper->SetInputData(poly.GetPointer());
    polyDataMapper->SetTransformCoordinate(boxCoordinate.GetPointer());
    polyDataMapper->SetTransformCoordinateUseDouble(true);

    vtkNew<vtkActor2D> boxActor;
    boxActor->SetMapper(polyDataMapper.GetPointer());

    renderer->AddActor2D(boxActor.GetPointer());

    renderWindow->AddRenderer(renderer.GetPointer());

    if ( i % 2 )
      {
      x += width;
      y -= height;
      height *= 2.;
      }
    else
      {
      x -= width;
      y += height;
      width *= 2.;
      }
    }

  //Render and interact
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow.GetPointer());

  renderWindow->SetMultiSamples(0);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow.GetPointer());
  if(retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    interactor->Start();
    }

  return !retVal;
}
