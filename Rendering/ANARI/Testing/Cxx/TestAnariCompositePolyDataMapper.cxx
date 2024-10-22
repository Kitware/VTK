// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkCullerCollection.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkTrivialProducer.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkAnariPass.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestInteractor.h"
#include "vtkAnariTestUtilities.h"

#include "vtkCylinderSource.h"

int TestAnariCompositePolyDataMapper(int argc, char* argv[])
{
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_WARNING);
  bool useDebugDevice = true;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-trace"))
    {
      useDebugDevice = true;
      vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
    }
  }

  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderer> ren;
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  vtkNew<vtkCompositePolyDataMapper> mapper;
  vtkNew<vtkCompositeDataDisplayAttributes> cdsa;
  mapper->SetCompositeDataDisplayAttributes(cdsa);

  int resolution = 18;
  vtkNew<vtkCylinderSource> cyl;
  cyl->CappingOn();
  cyl->SetRadius(0.2);
  cyl->SetResolution(resolution);

  // build a composite dataset
  vtkNew<vtkMultiBlockDataSet> data;
  //  int blocksPerLevel[3] = {1,64,256};
  int blocksPerLevel[3] = { 1, 16, 32 };
  std::vector<vtkSmartPointer<vtkMultiBlockDataSet>> blocks;
  blocks.push_back(data);
  unsigned levelStart = 0;
  unsigned levelEnd = 1;
  int numLevels = sizeof(blocksPerLevel) / sizeof(blocksPerLevel[0]);
  int numLeaves = 0;
  int numNodes = 0;
  vtkStdString blockName("Rolf");
  mapper->SetInputDataObject(data);
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
          cyl->Update();
          child->DeepCopy(cyl->GetOutput(0));
          blocks[parent]->SetBlock(block, (block % 2) ? nullptr : child.GetPointer());
          blocks[parent]->GetMetaData(block)->Set(vtkCompositeDataSet::NAME(), blockName.c_str());
          // test not setting it on some
          if (block % 11)
          {
            double hsv[3] = { 0.8 * block / nblocks, 0.2 + 0.8 * ((parent - levelStart) % 8) / 7.0,
              1.0 };
            double rgb[3];
            vtkMath::HSVToRGB(hsv, rgb);
            mapper->SetBlockColor(parent + numLeaves + 1, rgb);
            mapper->SetBlockOpacity(parent + numLeaves, (block + 3) % 7 == 0 ? 0.3 : 1.0);
            mapper->SetBlockVisibility(parent + numLeaves, (block % 7) != 0);
          }
          ++numLeaves;
        }
        else
        {
          vtkNew<vtkMultiBlockDataSet> child;
          blocks[parent]->SetBlock(block, child);
          blocks.push_back(child);
        }
      }
    }
    levelStart = levelEnd;
    levelEnd = static_cast<unsigned>(blocks.size());
  }

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  // actor->GetProperty()->SetEdgeColor(1,0,0);
  // actor->GetProperty()->EdgeVisibilityOn();
  ren->AddActor(actor);
  win->SetSize(400, 400);

  ren->RemoveCuller(ren->GetCullers()->GetLastItem());
  vtkNew<vtkAnariPass> anariPass;
  ren->SetPass(anariPass);

  SetParameterDefaults(anariPass, ren, useDebugDevice, "TestAnariCompositePolyDataMapper");

  ren->ResetCamera();
  vtkNew<vtkTimerLog> timer;
  win->Render(); // get the window up

  timer->StartTimer();
  win->Render();
  timer->StopTimer();
  cout << "First frame time: " << timer->GetElapsedTime() << "\n";

  timer->StartTimer();

  int numFrames = 2;
  for (int i = 0; i <= numFrames; i++)
  {
    ren->GetActiveCamera()->Elevation(40.0 / numFrames);
    ren->GetActiveCamera()->Zoom(pow(2.0, 1.0 / numFrames));
    ren->GetActiveCamera()->Roll(20.0 / numFrames);
    win->Render();
  }

  timer->StopTimer();
  double t = timer->GetElapsedTime();
  cout << "Avg Frame time: " << t / numFrames << " Frame Rate: " << numFrames / t << "\n";

  int retVal = vtkRegressionTestImage(win);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkAnariTestInteractor> style;
    style->SetPipelineControlPoints(ren, anariPass, nullptr);
    iren->SetInteractorStyle(style);
    style->SetCurrentRenderer(ren);

    iren->Start();
  }

  return !retVal;
}
