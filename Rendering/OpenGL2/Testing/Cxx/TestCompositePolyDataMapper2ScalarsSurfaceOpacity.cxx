// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Hide VTK_DEPRECATED_IN_9_3_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkCullerCollection.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderingOpenGLConfigure.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkTrivialProducer.h"

#include <vtkRegressionTestImage.h>
#include <vtkTestUtilities.h>

#include "vtkCylinderSource.h"
#include "vtkElevationFilter.h"

// This test exercises the vtkCompositePolyDataMapper2's ability to
// render scalars with surface opacity mapping enabled. In particular,
// it checks for correct rendering behavior when root blocks are
// set to invisible but sub-blocks are set to visible.
int TestCompositePolyDataMapper2ScalarsSurfaceOpacity(int argc, char* argv[])
{
  bool timeit = false;
  if (argc > 1 && argv[1] && !strcmp(argv[1], "-timeit"))
  {
    timeit = true;
  }

  vtkSmartPointer<vtkRenderWindow> win = vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
  win->AddRenderer(ren);
  win->SetInteractor(iren);
  ren->SetBackground(1.0, 1.0, 1.0);

  vtkSmartPointer<vtkCompositePolyDataMapper2> mapper =
    vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  vtkNew<vtkCompositeDataDisplayAttributes> cdsa;
  mapper->SetCompositeDataDisplayAttributes(cdsa.GetPointer());

  int resolution = 18;
  vtkNew<vtkCylinderSource> cyl;
  cyl->CappingOn();
  cyl->SetRadius(0.2);
  cyl->SetResolution(resolution);

  vtkNew<vtkElevationFilter> elev;
  elev->SetInputConnection(cyl->GetOutputPort());
  // geometry range is -0.5 to 0.5 but these colors are
  // pretty
  elev->SetLowPoint(0, -1.0, 0);
  elev->SetHighPoint(0.0, 1.0, 0.0);

  // build a composite dataset
  vtkNew<vtkMultiBlockDataSet> data;
  int blocksPerLevel[3] = { 1, 32, 64 };
  if (timeit)
  {
    blocksPerLevel[1] = 64;
    blocksPerLevel[2] = 256;
  }
  std::vector<vtkSmartPointer<vtkMultiBlockDataSet>> blocks;
  blocks.emplace_back(data.GetPointer());
  unsigned levelStart = 0;
  unsigned levelEnd = 1;
  int numLevels = sizeof(blocksPerLevel) / sizeof(blocksPerLevel[0]);
  int numLeaves = 0;
  int numNodes = 0;
  std::string blockName("Rolf");
  mapper->SetInputDataObject(data.GetPointer());
  for (int level = 1; level < numLevels; ++level)
  {
    int nblocks = blocksPerLevel[level];
    for (unsigned parent = levelStart; parent < levelEnd; ++parent)
    {
      blocks[parent]->SetNumberOfBlocks(nblocks);
      for (int block = 0; block < nblocks; ++block, ++numNodes)
      {
        if (level == numLevels - 1)
        {
          vtkNew<vtkPolyData> child;
          cyl->SetCenter(block * 0.25, 0.0, parent * 0.5);
          elev->Update();
          child->DeepCopy(elev->GetOutput(0));
          blocks[parent]->SetBlock(block, (block % 2) ? nullptr : child.GetPointer());
          blocks[parent]->GetMetaData(block)->Set(vtkCompositeDataSet::NAME(), blockName.c_str());

          // make children explicitly visible except every 11th
          mapper->SetBlockVisibility(parent + numLeaves, numLeaves % 11 != 0);
          ++numLeaves;
        }
        else
        {
          vtkNew<vtkMultiBlockDataSet> child;
          blocks[parent]->SetBlock(block, child.GetPointer());
          blocks.emplace_back(child.GetPointer());
        }
      }
    }
    levelStart = levelEnd;
    levelEnd = static_cast<unsigned>(blocks.size());
  }

  double range[2];
  elev->GetOutput(0)->GetScalarRange(range);

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  ren->AddActor(actor);
  win->SetSize(400, 400);

  ren->RemoveCuller(ren->GetCullers()->GetLastItem());
  ren->ResetCamera();

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  win->Render(); // get the window up

  // Set up discretizable color transfer function with opacity enable
  const double controlPoints[] = { range[0], 1.0, 0.0, 0.0, range[1], 0.0, 0.0, 1.0 };

  vtkSmartPointer<vtkDiscretizableColorTransferFunction> dctf =
    vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();
  for (int i = 0; i < 2; ++i)
  {
    const double* xrgb = controlPoints + (i * 4);
    dctf->AddRGBPoint(xrgb[0], xrgb[1], xrgb[2], xrgb[3]);
  }

  // Scalar opacity transfer function
  const double opacityControlPoints[] = { range[0], 0.1, range[1], 1.0 };

  vtkSmartPointer<vtkPiecewiseFunction> pf = vtkSmartPointer<vtkPiecewiseFunction>::New();
  for (int i = 0; i < 2; ++i)
  {
    const double* xalpha = opacityControlPoints + (i * 2);
    pf->AddPoint(xalpha[0], xalpha[1]);
  }

  // Enable opacity mapping
  dctf->SetScalarOpacityFunction(pf);
  dctf->EnableOpacityMappingOn();
  dctf->Build();

  mapper->SetLookupTable(dctf);

  // modify the data to force a rebuild of OpenGL structs
  // after rendering set one cylinder to white
  mapper->SetBlockColor(911, 1.0, 1.0, 1.0);
  mapper->SetBlockOpacity(911, 1.0);

  // set intermediate block invisible
  mapper->SetBlockVisibility(911, false);

  // set root block visibility to false. Since visibility of children are
  // explicitly set, this should make no difference in rendering.
  mapper->SetBlockVisibility(0, false);

  // set a block not visible
  mapper->SetBlockVisibility(912, false);

  timer->StartTimer();
  win->Render();
  timer->StopTimer();
  cout << "First frame time: " << timer->GetElapsedTime() << "\n";

  timer->StartTimer();

  int numFrames = (timeit ? 300 : 2);
  for (int i = 0; i <= numFrames; i++)
  {
    ren->GetActiveCamera()->Elevation(40.0 / numFrames);
    ren->GetActiveCamera()->Zoom(pow(2.0, 1.0 / numFrames));
    ren->GetActiveCamera()->Roll(20.0 / numFrames);
    win->Render();
  }

  timer->StopTimer();
  if (timeit)
  {
    double t = timer->GetElapsedTime();
    cout << "Avg Frame time: " << t / numFrames << " Frame Rate: " << numFrames / t << "\n";
  }

  int retVal = vtkRegressionTestImageThreshold(win.GetPointer(), 15);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
