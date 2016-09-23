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
//  +---+  INF  Red
//  |   |
//  +---+  1.0  Red
//  |   |
//  +---+  0.5  Green
//  |   |
//  +---+  NAN  Magenta
//  |   |
//  +---+  0.5  Green
//  |   |
//  +---+  0.0  Blue
//  |   |
//  +---+  -INF Blue
//
// These values are mapped to the spectrum colors from blue (low) to red (high).
// -INF should be blue, INF should be red.  Since these are near extrema,
// whatever interpolation used should be constant.  NAN should be drawn as
// magenta.  The interpolation to NAN is ill defined in a texture map.  I would
// expect a sharp transition to the NAN color, but that might depend on graphics
// hardware.

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkColorTransferFunction.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkDoubleArray.h"
#include "vtkLookupTable.h"
#include "vtkLogLookupTable.h"
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
  const int cellsHigh = 6;
  const int pointsHigh = cellsHigh + 1;
  const double pointValues[pointsHigh] = {
    vtkMath::NegInf(), 0.0, 0.5, vtkMath::Nan(), 0.5, 1.0, vtkMath::Inf()
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

static vtkSmartPointer<vtkLookupTable> CreateLookupTable()
{
  VTK_CREATE(vtkLookupTable, lut);

  lut->SetRampToLinear();
  lut->SetScaleToLinear();
  lut->SetTableRange(0.0, 1.0);
  lut->SetHueRange(0.6, 0.0);
  lut->SetNanColor(1.0, 0.0, 1.0, 1.0);

  return lut;
}

static vtkSmartPointer<vtkLogLookupTable> CreateLogLookupTable()
{
  VTK_CREATE(vtkLogLookupTable, lut);

  lut->SetRampToLinear();
  lut->SetScaleToLinear();
  lut->SetTableRange(0.0, 1.0);
  lut->SetHueRange(0.6, 0.0);
  lut->SetNanColor(1.0, 0.0, 1.0, 1.0);

  return lut;
}

static vtkSmartPointer<vtkColorTransferFunction> CreateColorTransferFunction()
{
  VTK_CREATE(vtkColorTransferFunction, ctf);

  ctf->SetColorSpaceToHSV();
  ctf->HSVWrapOff();
  ctf->AddHSVSegment(0.0, 0.6, 1.0, 1.0,
                     1.0, 0.0, 1.0, 1.0);
  ctf->SetNanColor(1.0, 0.0, 1.0);

  return ctf;
}

static vtkSmartPointer<vtkDiscretizableColorTransferFunction> CreateDiscretizableColorTransferFunction()
{
  VTK_CREATE(vtkDiscretizableColorTransferFunction, ctf);

  ctf->DiscretizeOn();
  ctf->SetColorSpaceToHSV();
  ctf->HSVWrapOff();
  ctf->AddHSVSegment(0.0, 0.6, 1.0, 1.0,
                     1.0, 0.0, 1.0, 1.0);
  ctf->SetNanColor(1.0, 0.0, 1.0);
  ctf->Build();

  return ctf;
}

static vtkSmartPointer<vtkRenderer> CreateRenderer(vtkPolyData *input,
                                                   vtkScalarsToColors *lut,
                                                   int interpolate)
{
  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputData(input);
  mapper->SetLookupTable(lut);
  mapper->SetInterpolateScalarsBeforeMapping(interpolate);

  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);

  VTK_CREATE(vtkRenderer, renderer);
  renderer->AddActor(actor);
  renderer->ResetCamera();

  return renderer;
}

const int NUM_RENDERERS = 8;
static void AddRenderer(vtkRenderer *renderer, vtkRenderWindow *renwin)
{
  static int rencount = 0;
  renderer->SetViewport(static_cast<double>(rencount)/NUM_RENDERERS, 0.0,
                        static_cast<double>(rencount+1)/NUM_RENDERERS, 1.0);
  renwin->AddRenderer(renderer);
  rencount++;
}

int RenderNonFinite(int argc, char *argv[])
{
  vtkSmartPointer<vtkPolyData> input = CreateData();

  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->SetSize(300, 200);

  vtkSmartPointer<vtkRenderer> renderer;

  renderer = CreateRenderer(input, CreateLookupTable(), 0);
  AddRenderer(renderer, renwin);

  renderer = CreateRenderer(input, CreateLookupTable(), 1);
  AddRenderer(renderer, renwin);

  renderer = CreateRenderer(input, CreateLogLookupTable(), 0);
  AddRenderer(renderer, renwin);

  renderer = CreateRenderer(input, CreateLogLookupTable(), 1);
  AddRenderer(renderer, renwin);

  renderer = CreateRenderer(input, CreateColorTransferFunction(), 0);
  AddRenderer(renderer, renwin);

  renderer = CreateRenderer(input, CreateColorTransferFunction(), 1);
  AddRenderer(renderer, renwin);

  renderer = CreateRenderer(input, CreateDiscretizableColorTransferFunction(), 0);
  AddRenderer(renderer, renwin);

  renderer = CreateRenderer(input, CreateDiscretizableColorTransferFunction(), 1);
  AddRenderer(renderer, renwin);

  renwin->Render();

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
