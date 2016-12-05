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
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnsignedCharArray.h"

int TestGlyph3DMapperTreeIndexingCompositeGlyphs(int argc, char *argv[])
{
  // The points to glyph:
  vtkNew<vtkPolyData> input;
  vtkNew<vtkPoints> points;
  vtkNew<vtkIntArray> indexArray;
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(3);

  unsigned char color[3];
  for (int row = 0; row < 2; ++row)
  {
    for (int col = 0; col < 3; ++col)
    {
      points->InsertNextPoint((row ? col : (2 - col)) * 5, row * 5, 0.);
      indexArray->InsertNextValue(col);

      color[0] = static_cast<unsigned char>(((row + 1) / 2.) * 255 + 0.5);
      color[1] = static_cast<unsigned char>(((col + 1) / 3.) * 255 + 0.5);
      color[2] = static_cast<unsigned char>(((row + col + 1) / 4.) * 255 + 0.5);
      colors->InsertNextTypedTuple(color);
    }
  }

  input->SetPoints(points.Get());
  input->GetPointData()->AddArray(indexArray.Get());
  indexArray->SetName("GlyphIndex");
  input->GetPointData()->AddArray(colors.Get());
  colors->SetName("Colors");


  // The glyph sources:
  vtkNew<vtkArrowSource> s0a;
  vtkNew<vtkTransformFilter> s0b;
  vtkNew<vtkCubeSource> s1a;
  vtkNew<vtkTransformFilter> s1b;
  vtkNew<vtkSphereSource> s2a;
  vtkNew<vtkTransformFilter> s2b;

  vtkNew<vtkTransform> transform;
  transform->Identity();
  transform->RotateZ(45.);
  transform->Scale(0.5, 2, 1.0);
  transform->Translate(.5, .5, .5);

  s0b->SetInputConnection(s0a->GetOutputPort());
  s0b->SetTransform(transform.Get());
  s1b->SetInputConnection(s1a->GetOutputPort());
  s1b->SetTransform(transform.Get());
  s2b->SetInputConnection(s2a->GetOutputPort());
  s2b->SetTransform(transform.Get());

  s0a->Update();
  s0b->Update();
  s1a->Update();
  s1b->Update();
  s2a->Update();
  s2b->Update();

  vtkNew<vtkMultiBlockDataSet> s0;
  s0->SetNumberOfBlocks(2);
  s0->SetBlock(0, s0a->GetOutputDataObject(0));
  s0->SetBlock(1, s0b->GetOutputDataObject(0));

  vtkNew<vtkMultiBlockDataSet> s1;
  s1->SetNumberOfBlocks(2);
  s1->SetBlock(0, s1a->GetOutputDataObject(0));
  s1->SetBlock(1, s1b->GetOutputDataObject(0));

  vtkNew<vtkMultiBlockDataSet> s2;
  s2->SetNumberOfBlocks(2);
  s2->SetBlock(0, s2a->GetOutputDataObject(0));
  s2->SetBlock(1, s2b->GetOutputDataObject(0));

  // Combine the glyph sources into a single dataset:
  vtkNew<vtkMultiBlockDataSet> glyphTree;
  glyphTree->SetNumberOfBlocks(3);
  glyphTree->SetBlock(0, s0.Get());
  glyphTree->SetBlock(1, s1.Get());
  glyphTree->SetBlock(2, s2.Get());

  vtkNew<vtkGlyph3DMapper> mapper;
  mapper->SetInputData(input.Get());
  mapper->SetSourceTableTree(glyphTree.Get());
  mapper->SetRange(0, 2);
  mapper->SetUseSourceTableTree(true);
  mapper->SetSourceIndexing(true);
  mapper->SetSourceIndexArray("GlyphIndex");
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("Colors");

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
