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
#include "vtkPointDataToCellData.h"

#include <vtkTestUtilities.h>
#include <vtkRegressionTestImage.h>

#include "vtkCylinderSource.h"
#include "vtkElevationFilter.h"

int TestCompositePolyDataMapper2CellScalars(int argc, char* argv[])
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

  win->SetMultiSamples(0);

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

  vtkNew<vtkPointDataToCellData> p2c;
  p2c->SetInputConnection(elev->GetOutputPort());
  p2c->PassPointDataOff();

  // build a composite dataset
  vtkNew<vtkMultiBlockDataSet> data;
  int blocksPerLevel[3] = {1,32,64};
  if (timeit)
  {
    blocksPerLevel[1] = 64;
    blocksPerLevel[2] = 256;
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
          elev->SetLowPoint(block*0.25 - 0.2 + 0.2*block/nblocks, -0.02, 0.0);
          elev->SetHighPoint(block*0.25 + 0.1 + 0.2*block/nblocks, 0.02, 0.0);
          p2c->Update();
          child->DeepCopy(p2c->GetOutput(0));
          blocks[parent]->SetBlock(
            block, (block % 2) ? NULL : child.GetPointer());
          blocks[parent]->GetMetaData(block)->Set(
            vtkCompositeDataSet::NAME(), blockName.c_str());
          // test not seting it on some
          if (block % 11)
          {
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
  mapper->SetScalarModeToUseCellData();

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetEdgeColor(1,0,0);
  //actor->GetProperty()->EdgeVisibilityOn();
  ren->AddActor(actor);
  win->SetSize(400,400);

  ren->RemoveCuller(ren->GetCullers()->GetLastItem());
  ren->ResetCamera();

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  win->Render();  // get the window up

  // modify the data to force a rebuild of OpenGL structs
  // after rendering set one cylinder to white
  mapper->SetBlockColor(911,1.0,1.0,1.0);
  mapper->SetBlockOpacity(911,1.0);
  mapper->SetBlockVisibility(911,1.0);

  timer->StartTimer();
  win->Render();
  timer->StopTimer();
  cout << "First frame time: " << timer->GetElapsedTime() << "\n";

  timer->StartTimer();

  int numFrames = (timeit ? 300 : 2);
  for (int i = 0; i <= numFrames; i++)
  {
    ren->GetActiveCamera()->Elevation(10.0/numFrames);
    ren->GetActiveCamera()->Azimuth(-50.0/numFrames);
    ren->GetActiveCamera()->Zoom(pow(2.5,1.0/numFrames));
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
