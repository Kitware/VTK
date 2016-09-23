/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TimeRenderer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkConeSource.h"
#include "vtkCubeSource.h"
#include "vtkCullerCollection.h"
#include "vtkGarbageCollector.h"
#include "vtkGlyph3D.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkStripper.h"
#include "vtkTimerLog.h"
#include "vtkTriangleFilter.h"

int main( int argc, char *argv[] )
{
  // For timings
  int i;
  int l, w, nActors, N, n;
  vtkIdType aPnts;

  if (argc != 5)
  {
    l = 10;
    w = 10;
    aPnts = 15;
    nActors = 100;
  }
  else
  {
    l = atoi(argv[1]);
    w = atoi(argv[2]);
    aPnts = atoi(argv[3]);
    nActors = atoi(argv[4]);
  }

  // n is the number of points per level
  n = l * w;
  // N is the total number of points
  N = aPnts * nActors;

  float x, y, z;
  vtkIdType *cdata = new vtkIdType [aPnts];
  for (int j = 0; j < aPnts; j++)
  {
    cdata[j] = j;
  }

  vtkProperty *prop = vtkProperty::New();

  //vtkGarbageCollector::DeferredCollectionPush();

  // create a rendering window and both renderers
  vtkRenderer *ren1 = vtkRenderer::New();
  ren1->GetCullers()->InitTraversal();
  vtkRenderWindow *renWindow = vtkRenderWindow::New();
  renWindow->AddRenderer(ren1);

  // Create a cube polydata
  vtkPoints *cpnts = vtkPoints::New();
  cpnts->SetNumberOfPoints(14);

  vtkCellArray *ccells = vtkCellArray::New();

  cpnts->SetPoint(0,   .1, -.1, -.1);
  cpnts->SetPoint(1,  -.1, -.1, -.1);
  cpnts->SetPoint(2,   .1,  .1, -.1);
  cpnts->SetPoint(3,  -.1,  .1, -.1);
  cpnts->SetPoint(4,  -.1,  .1,  .1);
  cpnts->SetPoint(5,  -.1, -.1, -.1);
  cpnts->SetPoint(6,  -.1, -.1,  .1);
  cpnts->SetPoint(7,   .1, -.1, -.1);
  cpnts->SetPoint(8,   .1, -.1,  .1);
  cpnts->SetPoint(9,   .1,  .1, -.1);
  cpnts->SetPoint(10,  .1,  .1,  .1);
  cpnts->SetPoint(11, -.1,  .1,  .1);
  cpnts->SetPoint(12,  .1, -.1,  .1);
  cpnts->SetPoint(13, -.1, -.1,  .1);

  vtkIdType a[14];
  for (i = 0; i < 14; i++)
  {
    a[i] = i;
  }

  ccells->InsertNextCell(14L, a);
  ccells->Squeeze();

  vtkPolyData *cube = vtkPolyData::New();
  cube->SetPoints(cpnts);
  cube->SetStrips(ccells);
  cpnts->Delete();
  ccells->Delete();

  vtkPolyDataMapper *mapper;
  vtkCellArray *cells;
  vtkActor *actor;
  vtkGlyph3D *filter;
  vtkPolyData *data;
  vtkPoints *pnts = 0;
  vtkTriangleFilter *tfilter;
  vtkStripper *stripper;

  x = 0.0;
  y = 0.0;
  z = 0.0;
  for (i = 0; i < N; i ++)
  {
    // See if we need to start a new actor
    if ((i % aPnts) == 0)
    {
      if (pnts)
      {
        pnts->Delete();
      }

      pnts = vtkPoints::New();
      cells = vtkCellArray::New();
      data = vtkPolyData::New();
      filter = vtkGlyph3D::New();
      mapper = vtkPolyDataMapper::New();
      actor = vtkActor::New();
      tfilter = vtkTriangleFilter::New();
      stripper = vtkStripper::New();

      prop->SetInterpolationToFlat();
      actor->SetProperty(prop);

      pnts->SetNumberOfPoints(aPnts);
      cells->Allocate(aPnts);
      cells->InsertNextCell(aPnts, cdata);
      data->SetVerts(cells);
      data->SetPoints(pnts);
      tfilter->SetInputData(cube);
      stripper->SetInputConnection(tfilter->GetOutputPort());
      filter->SetSourceConnection(stripper->GetOutputPort());
      filter->SetInputData(data);
      mapper->SetInputConnection(filter->GetOutputPort());
      actor->SetMapper(mapper);
      ren1->AddActor(actor);

      // all these are held by way of the renderer, so do a fast unref
      cells->Delete();
      data->Delete();
      filter->Delete();
      mapper->Delete();
      actor->Delete();
      tfilter->Delete();
      stripper->Delete();
    }

    // See if we are on a new level)
    if ((i % n) == 0)
    {
      z += 1.0;
      x = 0.0;
      y = 0.0;
    }
    else
    {
      if ((i % l) == 0)
      {
        x += 1.0;
        y = 0.0;
      }
      else
      {
        y += 1.0;
      }
    }

    pnts->SetPoint(i % aPnts, x, y, z);
    pnts->Modified();
  }

  if (pnts)
  {
    pnts->Delete();
  }

  // set the size of our window
  renWindow->SetSize(500,500);

  // set the viewports and background of the renderers
  //  ren1->SetViewport(0,0,0.5,1);
  ren1->SetBackground(0.2,0.3,0.5);

  // draw the resulting scene
  renWindow->Render();
  ren1->GetActiveCamera()->Azimuth(3);
  renWindow->Render();

  // Set up times
  vtkTimerLog *tl = vtkTimerLog::New();

  tl->StartTimer();

  // do a azimuth of the cameras 3 degrees per iteration
  // for (i = 0; i < 360; i += 3)
#if 1
  for (i = 0; i < 360; i += 9)
  {
    ren1->GetActiveCamera()->Azimuth(3);
    renWindow->Render();
  }
#endif
  tl->StopTimer();

  cerr << "Wall Time = " << tl->GetElapsedTime() << "\n";
  cerr << "FrameRate = " << 120.0 / tl->GetElapsedTime() << "\n";

  // Clean up
  cube->Delete();
  vtkGarbageCollector::SetGlobalDebugFlag(true);
  vtkGarbageCollector::SetGlobalDebugFlag(false);
  prop->Delete();
  ren1->Delete();
  renWindow->Delete();
  tl->Delete();
  //vtkGarbageCollector::DeferredCollectionPop();

  delete [] cdata;
  return 1;
}
