/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SurfacePlusEdges.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// This test draws a stick with non-finite values.  The topology of the stick is
// as follows.
//
//  +---+  NAN
//  |   |
//  +---+  INF
//  |   |
//  +---+  1.0
//  |   |
//  +---+  0.5
//  |   |
//  +---+  0.0
//  |   |
//  +---+  -INF
//
// These values are mapped to the spectrum colors from red (low) to blue (high).
// -INF should be blue, INF should be red.  Since these are near extrema,
// whatever interpolation used should be constant.  NAN should be drawn as grey.
// The interpolation to NAN is ill defined in a texture map.  I would expect a
// sharp transition to the NAN color, but that might depend on graphics
// hardware.

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, var) \
  vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

// Create the data described above.
static vtkSmartPointer<vtkPolyData> CreateData()
{
  const int cellsHigh = 5;
  const int pointsHigh = cellsHigh + 1;
  const double pointValues[pointsHigh] = {
    vtkMath::NegInf(), 0.0, 0.5, 1.0, vtkMath::Inf(), vtkMath::Nan()
  };

  VTK_CREATE(vtkPolyData, polyData);

  VTK_CREATE(vtkPoints, points);
  for (int y = 0; y < pointsHigh; y++)
    {
    for (int x = 0; x < 2; x++)
      {
      points->InsertNextPoint(static_cast<double>(x),
                              static_cast<double>(y), 0.0);
      }
    }
  polyData->SetPoints(points);

  VTK_CREATE(vtkCellArray, cells);
  for (int c = 0; c < cellsHigh; c++)
    {
    cells->InsertNextCell(4);
    cells->InsertCellPoint(2*c);
    cells->InsertCellPoint(2*c+1);
    cells->InsertCellPoint(2*c+3);
    cells->InsertCellPoint(2*c+2);
    }
  polyData->SetPolys(cells);

  VTK_CREATE(vtkDoubleArray, scalars);
  for (int height = 0; height < pointsHigh; height++)
    {
    scalars->InsertNextTuple1(pointValues[height]);
    scalars->InsertNextTuple1(pointValues[height]);
    }
  polyData->GetPointData()->SetScalars(scalars);

  return polyData;
}

static vtkSmartPointer<vtkRenderer> CreateRenderer(vtkPolyData *input,
                                                   int interpolate)
{
  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInput(input);
  mapper->SetInterpolateScalarsBeforeMapping(interpolate);

  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);

  VTK_CREATE(vtkRenderer, renderer);
  renderer->AddActor(actor);
  renderer->ResetCamera();

  return renderer;
}

int RenderNonFinite(int argc, char *argv[])
{
  vtkSmartPointer<vtkPolyData> input = CreateData();

  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->SetSize(200, 200);

  vtkSmartPointer<vtkRenderer> renderer;

  renderer = CreateRenderer(input, 0);
  renderer->SetViewport(0.0, 0.0, 0.5, 1.0);
  renwin->AddRenderer(renderer);

  renderer = CreateRenderer(input, 1);
  renderer->SetViewport(0.5, 0.0, 1.0, 1.0);
  renwin->AddRenderer(renderer);

  int retVal = vtkRegressionTestImage(renwin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    VTK_CREATE(vtkRenderWindowInteractor, iren);
    iren->SetRenderWindow(renwin);
    iren->Initialize();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
    }

  return (retVal == vtkRegressionTester::PASSED) ? 0 : 1;
}
