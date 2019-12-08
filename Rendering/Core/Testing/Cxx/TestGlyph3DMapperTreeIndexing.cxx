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
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

int TestGlyph3DMapperTreeIndexing(int argc, char* argv[])
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

  input->SetPoints(points);
  input->GetPointData()->AddArray(indexArray);
  indexArray->SetName("GlyphIndex");

  // The glyph sources:
  vtkNew<vtkArrowSource> s0;
  vtkNew<vtkCubeSource> s1;
  vtkNew<vtkSphereSource> s2;
  s0->Update();
  s1->Update();
  s2->Update();

  // Combine the glyph sources into a single dataset:
  vtkNew<vtkMultiBlockDataSet> glyphTree;
  glyphTree->SetNumberOfBlocks(3);
  glyphTree->SetBlock(0, s0->GetOutputDataObject(0));
  glyphTree->SetBlock(1, s1->GetOutputDataObject(0));
  glyphTree->SetBlock(2, s2->GetOutputDataObject(0));

  vtkNew<vtkGlyph3DMapper> mapper;
  mapper->SetInputData(input);
  mapper->SetSourceTableTree(glyphTree);
  mapper->SetRange(0, 2);
  mapper->SetUseSourceTableTree(true);
  mapper->SetSourceIndexing(true);
  mapper->SetSourceIndexArray("GlyphIndex");

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1., 0., 0.);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0., 0., 0.);
  renderer->ResetCamera();
  renderer->ResetCameraClippingRange();
  renderer->AutomaticLightCreationOff();
  renderer->RemoveAllLights();

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);

  // Ensure the mapper works when no lights are available
  renderer->AutomaticLightCreationOff();
  renderer->RemoveAllLights();

  renWin->Render();

  renderer->AutomaticLightCreationOn();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
