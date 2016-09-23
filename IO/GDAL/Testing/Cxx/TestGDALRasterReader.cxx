/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGDALRasterReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkGDALRasterReader.h>

// VTK includes
#include <vtkImageActor.h>
#include <vtkCellData.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkLookupTable.h>
#include <vtkMapper.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>

// C++ includes
#include <sstream>

// Main program
int TestGDALRasterReader(int argc, char** argv)
{
  const char* rasterFileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                 "Data/GIS/raster.tif");

  // Create reader to read shape file.
  vtkNew<vtkGDALRasterReader> reader;
  reader->SetFileName(rasterFileName);
  reader->Update();
  delete [] rasterFileName;

  // We need a renderer
  vtkNew<vtkRenderer> renderer;

  // Get the data
  vtkNew<vtkImageActor> actor;
  actor->SetInputData(reader->GetOutput());
  renderer->AddActor(actor.GetPointer());

  // Create a render window, and an interactor
  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindow->AddRenderer(renderer.GetPointer());
  renderWindowInteractor->SetRenderWindow(renderWindow.GetPointer());

  //Add the actor to the scene
  renderer->SetBackground(1.0, 1.0, 1.0);
  renderWindow->SetSize(400, 400);
  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow.GetPointer());

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
