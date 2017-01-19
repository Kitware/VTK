/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGlyph3DMapper.h"

#include "vtkActor.h"
#include "vtkArrowSource.h"
#include "vtkCubeSource.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"

int TestGlyph3DMapperIndexing(int argc, char *argv[])
{
  // The points to glyph:
  vtkNew<vtkPolyData> input;
  vtkNew<vtkPoints> points;
  vtkNew<vtkIntArray> indexArray;

  for (int row = 0; row < 2; ++row)
  {
    for (int col = 0; col < 3; ++col)
    {
      points->InsertNextPoint((row ? col : (2 - col)) * 5, row * 5, 0.);
      indexArray->InsertNextValue(col);
    }
  }

  input->SetPoints(points.Get());
  input->GetPointData()->AddArray(indexArray.Get());
  indexArray->SetName("GlyphIndex");

  // The glyph sources:
  vtkNew<vtkArrowSource> s0;
  vtkNew<vtkCubeSource> s1;
  vtkNew<vtkSphereSource> s2;

  vtkNew<vtkGlyph3DMapper> mapper;
  mapper->SetInputData(input.Get());
  mapper->SetSourceConnection(0, s0->GetOutputPort());
  mapper->SetSourceConnection(1, s1->GetOutputPort());
  mapper->SetSourceConnection(2, s2->GetOutputPort());
  mapper->SetRange(0, 2);
  mapper->SetSourceIndexing(true);
  mapper->SetSourceIndexArray("GlyphIndex");

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetColor(1., 0., 0.);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor.Get());
  renderer->SetBackground(0., 0., 0.);
  renderer->ResetCamera();
  renderer->ResetCameraClippingRange();

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin.Get());
  renWin->AddRenderer(renderer.Get());
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin.Get());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
