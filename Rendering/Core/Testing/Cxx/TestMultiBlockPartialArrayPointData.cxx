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

#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkCylinderSource.h"
#include "vtkElevationFilter.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

// Test for multiblock data sets with field data arrays defined on
// only a subset of the blocks. The expected behavior is to have
// coloring by scalars on the blocks with the data array and coloring
// as though scalar mapping is turned off in the blocks without the
// data array.
int TestMultiBlockPartialArrayPointData(int argc, char* argv[])
{
  vtkSmartPointer<vtkRenderWindow> win =
    vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderer> ren =
    vtkSmartPointer<vtkRenderer>::New();
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  // Components of the multiblock data set
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetRadius(2.0);

  vtkNew<vtkCylinderSource> cylinderSource;
  cylinderSource->SetRadius(1.5);
  cylinderSource->SetHeight(2.0);
  cylinderSource->SetResolution(32);

  vtkNew<vtkElevationFilter> elevationFilter;
  elevationFilter->SetLowPoint(-10.0, 0.0, 0.0);
  elevationFilter->SetHighPoint( 10.0, 0.0, 0.0);
  elevationFilter->SetInputConnection(cylinderSource->GetOutputPort());

  // Set up the multiblock data set consisting of a ring of blocks
  vtkSmartPointer<vtkMultiBlockDataSet> data = vtkSmartPointer<vtkMultiBlockDataSet>::New();

  int numBlocks = 16;
  data->SetNumberOfBlocks(numBlocks);

  double radius = 10.0;
  double deltaTheta = 2.0*3.1415926 / numBlocks;
  for (int i = 0; i < numBlocks; ++i)
  {
    double theta = i * deltaTheta;
    double x = radius * cos(theta);
    double y = radius * sin(theta);

    vtkPolyData* pd = vtkPolyData::New();

    // Every third block does not have the color array
    if (i % 3 == 0)
    {
      sphereSource->SetCenter(x, y, 0.0);
      sphereSource->Update();
      pd->DeepCopy(sphereSource->GetOutput());
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

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.67, 1.0);

  ren->AddActor(actor);
  win->SetSize(400, 400);

  ren->ResetCamera();

  win->Render();

  int retVal = vtkRegressionTestImageThreshold( win.GetPointer(),15);
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
