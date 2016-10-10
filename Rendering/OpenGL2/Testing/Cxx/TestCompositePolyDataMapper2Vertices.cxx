/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositePolyDataMapper2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkCullerCollection.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkTrivialProducer.h"

#include <vtkTestUtilities.h>
#include <vtkRegressionTestImage.h>
#include "vtkCylinderSource.h"

int TestCompositePolyDataMapper2Vertices(int argc, char* argv[])
{
  bool timeit = false;
  if (argc > 1 && argv[1] && !strcmp(argv[1], "-timeit"))
  {
    timeit = true;
  }
  vtkSmartPointer<vtkRenderWindow> win =
    vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderer> ren =
    vtkSmartPointer<vtkRenderer>::New();
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  vtkSmartPointer<vtkCompositePolyDataMapper2> mapper =
    vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  vtkNew<vtkCompositeDataDisplayAttributes> cdsa;
  mapper->SetCompositeDataDisplayAttributes(cdsa.GetPointer());

  int resolution = 10;
  vtkNew<vtkCylinderSource> cyl;
  cyl->CappingOn();
  cyl->SetRadius(0.2);
  cyl->SetHeight(0.6);
  cyl->SetResolution(resolution);

  // build a composite dataset
  vtkNew<vtkMultiBlockDataSet> data;
  int blocksPerLevel[3] = {1,4,8};
  if (timeit)
  {
    blocksPerLevel[1] = 32;
    blocksPerLevel[2] = 64;
  }
  std::vector<vtkSmartPointer<vtkMultiBlockDataSet> > blocks;
  blocks.push_back(data.GetPointer());
  unsigned levelStart = 0;
  unsigned levelEnd = 1;
  int numLevels = sizeof(blocksPerLevel) / sizeof(blocksPerLevel[0]);
  int numLeaves = 0;
  int numNodes = 0;
  vtkStdString blockName("Rolf");
  for (int level = 1; level < numLevels; ++level)
  {
    int nblocks=blocksPerLevel[level];
    for (unsigned parent = levelStart; parent < levelEnd; ++parent)
    {
      blocks[parent]->SetNumberOfBlocks(nblocks);
      for (int block=0; block < nblocks; ++block, ++numNodes)
      {
        if (level == numLevels - 1)
        {
          vtkNew<vtkPolyData> child;
          cyl->SetCenter(block*0.25, 0.0, parent*0.5);
          cyl->Update();
          child->DeepCopy(cyl->GetOutput(0));
          blocks[parent]->SetBlock(
            block, (block % 2) ? NULL : child.GetPointer());
          blocks[parent]->GetMetaData(block)->Set(
            vtkCompositeDataSet::NAME(), blockName.c_str());
          // test not setting it on some
          if (block % 11)
          {
            mapper->SetBlockColor(parent+numLeaves+1,
              vtkMath::HSVToRGB(0.8*block/nblocks, 0.2 + 0.8*((parent - levelStart) % 8)/7.0, 1.0));
            mapper->SetBlockVisibility(parent+numLeaves, (block % 7) != 0);
          }
          ++numLeaves;
        }
        else
        {
          vtkNew<vtkMultiBlockDataSet> child;
          blocks[parent]->SetBlock(block, child.GetPointer());
          blocks.push_back(child.GetPointer());
        }
      }
    }
    levelStart = levelEnd;
    levelEnd = static_cast<unsigned>(blocks.size());
  }

  mapper->SetInputData((vtkPolyData *)(data.GetPointer()));

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->RenderLinesAsTubesOn();
  actor->GetProperty()->EdgeVisibilityOn();
  actor->GetProperty()->SetEdgeColor(0.7, 0.7, 0.7);
  actor->GetProperty()->SetLineWidth(7.0);
  actor->GetProperty()->RenderPointsAsSpheresOn();
  actor->GetProperty()->VertexVisibilityOn();
  actor->GetProperty()->SetVertexColor(1.0, 0.5, 0.5);
  actor->GetProperty()->SetPointSize(14.0);
  ren->AddActor(actor);

  win->SetSize(400,400);

  ren->RemoveCuller(ren->GetCullers()->GetLastItem());
  ren->ResetCamera();

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  win->Render();  // get the window up

  // modify the data to force a rebuild of OpenGL structs
  // after rendering set one cylinder to white
  mapper->SetBlockColor(1011,1.0,1.0,1.0);
  mapper->SetBlockOpacity(1011,1.0);
  mapper->SetBlockVisibility(1011,1.0);

  win->SetMultiSamples(0);
  timer->StartTimer();
  win->Render();
  timer->StopTimer();
  cout << "First frame time: " << timer->GetElapsedTime() << "\n";

  timer->StartTimer();

  int numFrames = (timeit ? 300 : 2);
  for (int i = 0; i <= numFrames; i++)
  {
    ren->GetActiveCamera()->Elevation(20.0/numFrames);
//    ren->GetActiveCamera()->Zoom(pow(2.0,1.0/numFrames));
    ren->GetActiveCamera()->Roll(20.0/numFrames);
    win->Render();
  }

  timer->StopTimer();
  if (timeit)
  {
    double t =  timer->GetElapsedTime();
    cout << "Avg Frame time: " << t/numFrames << " Frame Rate: " << numFrames / t << "\n";
  }
  int retVal = vtkRegressionTestImageThreshold( win.GetPointer(),15);
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
