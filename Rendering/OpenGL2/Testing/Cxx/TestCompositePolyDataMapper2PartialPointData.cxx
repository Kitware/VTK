// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Hide VTK_DEPRECATED_IN_9_3_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkCylinderSource.h"
#include "vtkElevationFilter.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

// Test for multiblock data sets with point data arrays defined on
// only a subset of the blocks. The expected behavior is to have
// coloring by scalars on the blocks with the data array and coloring
// as though scalar mapping is turned off in the blocks without the
// data array.
int TestCompositePolyDataMapper2PartialPointData(int argc, char* argv[])
{
  vtkSmartPointer<vtkRenderWindow> win = vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  // Components of the multiblock data set
  vtkNew<vtkCylinderSource> cylinderSource;
  cylinderSource->SetRadius(1.5);
  cylinderSource->SetHeight(2.0);
  cylinderSource->SetResolution(32);

  vtkNew<vtkElevationFilter> elevationFilter;
  elevationFilter->SetLowPoint(-10.0, 0.0, 0.0);
  elevationFilter->SetHighPoint(10.0, 0.0, 0.0);
  elevationFilter->SetInputConnection(cylinderSource->GetOutputPort());

  // Set up the multiblock data set consisting of a ring of blocks
  vtkSmartPointer<vtkMultiBlockDataSet> data = vtkSmartPointer<vtkMultiBlockDataSet>::New();

  int numBlocks = 15;
  data->SetNumberOfBlocks(numBlocks);

  double radius = 10.0;
  double deltaTheta = 2.0 * vtkMath::Pi() / numBlocks;
  for (int i = 0; i < numBlocks; ++i)
  {
    double theta = i * deltaTheta;
    double x = radius * cos(theta);
    double y = radius * sin(theta);

    vtkPolyData* pd = vtkPolyData::New();

    // Every third block does not have the color array
    if (i % 3 == 0)
    {
      cylinderSource->SetCenter(x, y, 0.0);
      cylinderSource->Update();
      pd->DeepCopy(cylinderSource->GetOutput());
      data->SetBlock(i, pd);
    }
    else
    {
      cylinderSource->SetCenter(x, y, 0.0);
      elevationFilter->Update();
      pd->DeepCopy(elevationFilter->GetOutput());
      data->SetBlock(i, pd);
    }
    pd->Delete();
  }

  vtkSmartPointer<vtkCompositePolyDataMapper2> mapper =
    vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  mapper->SetInputDataObject(data);

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.67, 1.0);

  ren->AddActor(actor);
  win->SetSize(400, 400);

  ren->ResetCamera();

  win->Render();

  int retVal = vtkRegressionTestImageThreshold(win, 15);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
