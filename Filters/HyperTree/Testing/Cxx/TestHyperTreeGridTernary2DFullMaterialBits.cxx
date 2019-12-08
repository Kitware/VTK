/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary2DMaterialBits.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay and Joachim Pouderoux, Kitware 2013
// This test was revised by Philippe Pebay, 2016
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkHyperTreeGridToDualGrid.h"

#include "vtkBitArray.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkContourFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarBarActor.h"
#include "vtkTextProperty.h"
#include "vtkTimerLog.h"
#include <sstream>

void GenerateDescriptorAndMaterial(
  int depth, int sx, int sy, int sz, int branch, std::stringstream& d, std::stringstream& m)
{
  vtkIdType l = sx * sy * sz;
  vtkIdType s = 1;
  for (int j = 0; j < depth; j++)
  {
    for (int i = 0; i < l; i++)
    {
      if (j > 0 && i % s == 0)
      {
        d << " ";
        if (j > 1)
        {
          m << " ";
        }
      }
      d << "R";
      if (j > 0)
      {
        m << "1";
      }
    }
    s *= branch * branch;
    l *= branch * branch;
    d << " |";
    if (j > 0)
    {
      m << " |";
    }
  }

  for (int i = 0; i < l; i++)
  {
    if (i % (s / branch) == 0)
    {
      d << " ";
      m << " ";
    }
    d << ".";
    m << "1";
  }
}

void GenerateDescriptorAndMaterial(
  int depth, int sx, int sy, int sz, int branch, vtkBitArray* d, vtkBitArray* m)
{
  vtkIdType l = sx * sy * sz;
  vtkIdType s = 1;
  for (int j = 0; j < depth - 1; j++)
  {
    for (int i = 0; i < l; i++)
    {
      d->InsertNextValue(1);
      if (j > 0)
      {
        m->InsertNextValue(1);
      }
    }
    s *= branch * branch;
    l *= branch * branch;
  }

  for (int i = 0; i < l; i++)
  {
    d->InsertNextValue(0);
    m->InsertNextValue(1);
  }
}

int TestHyperTreeGridTernary2DFullMaterialBits(int argc, char* argv[])
{
  int sx = 10;
  int sy = 10;
  int depth = 2;
  int branch = 3;

  vtkNew<vtkTimerLog> timer;

  vtkNew<vtkHyperTreeGridSource> htGrid;
  int maxLevel = depth;
  htGrid->SetMaxDepth(maxLevel);
  htGrid->SetDimensions(sx + 1, sy + 1, 1); // Dimension 2 in xy plane GridCell sx, sy, sz = 1
  htGrid->SetGridScale(1., 1., 1.);
  htGrid->SetBranchFactor(branch);
  htGrid->UseMaskOn();
  vtkNew<vtkIdTypeArray> zero;
  for (int i = 0; i < sx * sy; i++)
  {
    zero->InsertNextValue(i);
  }
  htGrid->SetLevelZeroMaterialIndex(zero);
  vtkNew<vtkBitArray> desc;
  vtkNew<vtkBitArray> mat;
  timer->StartTimer();
  cout << "Generating descriptors..." << endl;
  GenerateDescriptorAndMaterial(depth, sx, sy, 1, branch, desc, mat);
  timer->StopTimer();
  htGrid->SetDescriptorBits(desc);
  htGrid->SetMaskBits(mat);
  cout << " Done in " << timer->GetElapsedTime() << "s (" << desc->GetNumberOfTuples() << " nodes)"
       << endl;

  cout << "Constructing HTG " << sx << "x" << sy << "x" << 1 << "  branch: " << branch
       << "  depth: " << depth << "..." << endl;
  timer->StartTimer();
  htGrid->Update();
  timer->StopTimer();
  vtkHyperTreeGrid* ht = htGrid->GetHyperTreeGridOutput();
  cout << " Done in " << timer->GetElapsedTime() << "s" << endl;
  cout << "#pts " << ht->GetNumberOfVertices() << endl;
  timer->StartTimer();
  timer->StopTimer();

  cout << "HTG takes " << htGrid->GetOutput()->GetActualMemorySize() << "KB in memory." << endl;

  // Prepare an array of ids
  vtkNew<vtkIdTypeArray> idArray;
  idArray->SetName("Ids");
  idArray->SetNumberOfComponents(1);
  vtkIdType nbPoints = ht->GetNumberOfVertices();
  idArray->SetNumberOfValues(nbPoints);
  for (vtkIdType i = 0; i < nbPoints; ++i)
  {
    idArray->SetValue(i, i);
  }
  ht->GetPointData()->SetScalars(idArray);

  // Geometry
  cout << "Constructing geometry..." << endl;
  timer->StartTimer();
  vtkNew<vtkHyperTreeGridGeometry> geometry;
  geometry->SetInputData(htGrid->GetOutput());
  geometry->Update();
  vtkPolyData* pd = geometry->GetPolyDataOutput();
  timer->StopTimer();
  cout << " Done in " << timer->GetElapsedTime() << "s" << endl;

  vtkNew<vtkHyperTreeGridToDualGrid> h2ug;
  h2ug->SetInputData(htGrid->GetOutput());
  h2ug->Update();

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(geometry->GetOutputPort());
  mapper1->SetScalarRange(pd->GetCellData()->GetScalars()->GetRange());
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry->GetOutputPort());
  mapper2->ScalarVisibilityOff();
  vtkNew<vtkDataSetMapper> mapper3;
  mapper3->SetInputConnection(h2ug->GetOutputPort());
  mapper3->ScalarVisibilityOff();
  vtkNew<vtkDataSetMapper> mapper4;
  mapper4->SetInputConnection(h2ug->GetOutputPort());
  mapper4->ScalarVisibilityOff();

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);
  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.7, .7, .7);

  vtkNew<vtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetRepresentationToWireframe();
  actor3->GetProperty()->SetColor(.0, .0, .0);
  vtkNew<vtkActor> actor4;
  actor4->SetMapper(mapper4);
  actor4->GetProperty()->SetRepresentationToPoints();
  actor4->GetProperty()->SetPointSize(4);
  actor4->GetProperty()->SetColor(0., 1., 0.);

  vtkNew<vtkScalarBarActor> scalarBar;
  scalarBar->SetLookupTable(mapper1->GetLookupTable());
  scalarBar->SetLabelFormat("%.0f");
  scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar->GetPositionCoordinate()->SetValue(.80, .32);
  scalarBar->SetTitle("  id  ");
  scalarBar->SetNumberOfLabels(4);
  scalarBar->SetWidth(0.15);
  scalarBar->SetHeight(0.4);
  scalarBar->SetTextPad(4);
  scalarBar->SetMaximumWidthInPixels(60);
  scalarBar->SetMaximumHeightInPixels(200);
  scalarBar->SetTextPositionToPrecedeScalarBar();
  scalarBar->GetTitleTextProperty()->SetColor(.4, .4, .4);
  scalarBar->GetLabelTextProperty()->SetColor(.4, .4, .4);
  scalarBar->SetDrawFrame(1);
  scalarBar->GetFrameProperty()->SetColor(.4, .4, .4);
  scalarBar->SetDrawBackground(1);
  scalarBar->GetBackgroundProperty()->SetColor(1., 1., 1.);

  // Camera
  double bd[6];
  pd->GetBounds(bd);
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(pd->GetCenter());
  camera->SetPosition(.5 * bd[1], .5 * bd[3], 2 * bd[1]);

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->AddActor(actor3);
  renderer->AddActor(actor4);
  renderer->AddActor2D(scalarBar);

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

  int retVal = vtkRegressionTestImageThreshold(renWin, 70);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
