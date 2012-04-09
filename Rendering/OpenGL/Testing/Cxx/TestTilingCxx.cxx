/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTilingCxx.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtksys/SystemTools.hxx"

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"

#include "vtkActor2D.h"
#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkFloatArray.h"
#include "vtkImageMapper.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProgrammableAttributeDataFilter.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarsToColors.h"
#include "vtkSphereSource.h"
#include "vtkWindowToImageFilter.h"


#include "vtkSmartPointer.h"
#define VTK_CREATE(type, var) \
  vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

void colorCells(void *arg)
{
  VTK_CREATE(vtkMath, randomColorGenerator);
  vtkProgrammableAttributeDataFilter * randomColors =
    static_cast<vtkProgrammableAttributeDataFilter *>(arg);
  vtkPolyData * input =
    vtkPolyData::SafeDownCast(randomColors->GetInput());
  vtkPolyData * output = randomColors->GetPolyDataOutput();
  int numCells = input->GetNumberOfCells();
  VTK_CREATE(vtkFloatArray, colors);
  colors->SetNumberOfTuples(numCells);

  for(int i = 0; i < numCells; i++)
    {
    colors->SetValue(i, randomColorGenerator->Random(0 ,1));
    }

  output->GetCellData()->CopyScalarsOff();
  output->GetCellData()->PassData(input->GetCellData());
  output->GetCellData()->SetScalars(colors);
}

int TestTilingCxx(int argc, char* argv[])
{
  VTK_CREATE(vtkSphereSource, sphere);
  sphere->SetThetaResolution(20);
  sphere->SetPhiResolution(40);

  // Compute random scalars (colors) for each cell
  VTK_CREATE(vtkProgrammableAttributeDataFilter, randomColors);
  randomColors->SetInputConnection(sphere->GetOutputPort());
  randomColors->SetExecuteMethod(colorCells, randomColors);

  // mapper and actor
  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(randomColors->GetOutputPort());
  mapper->SetScalarRange(randomColors->GetPolyDataOutput()->GetScalarRange());

  VTK_CREATE(vtkActor, sphereActor);
  sphereActor->SetMapper(mapper);

  // Create a scalar bar
  VTK_CREATE(vtkScalarBarActor, scalarBar);
  scalarBar->SetLookupTable(mapper->GetLookupTable());
  scalarBar->SetTitle("Temperature");
  scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar->GetPositionCoordinate()->SetValue(0.1, 0.05);
  scalarBar->SetOrientationToVertical();
  scalarBar->SetWidth(0.8);
  scalarBar->SetHeight(0.9);
  scalarBar->SetLabelFormat("%-#6.3f");

  // Test the Get/Set Position
  scalarBar->SetPosition(scalarBar->GetPosition());

  // Create graphics stuff
  // Create the RenderWindow, Renderer and both Actors
  VTK_CREATE(vtkRenderer, ren1);
  VTK_CREATE(vtkRenderer, ren2);
  VTK_CREATE(vtkRenderWindow, renWin);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren1);
  renWin->AddRenderer(ren2);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renWin);

  ren1->AddActor(sphereActor);
  ren2->AddActor2D(scalarBar);
  renWin->SetSize(160, 160);
  ren1->SetViewport(0, 0, 0.75, 1.0);
  ren2->SetViewport(0.75, 0, 1.0, 1.0);
  ren2->SetBackground(0.3, 0.3, 0.3);

  // render the image
  scalarBar->SetNumberOfLabels(8);
  renWin->Render();
  renWin->Render(); // perform an extra render before capturing window

  vtksys::SystemTools::Delay(1000);

  VTK_CREATE(vtkWindowToImageFilter, w2i);
  w2i->SetInput(renWin);
  w2i->SetMagnification(2);
  w2i->Update();

  // copy the output
  vtkImageData * outputData = w2i->GetOutput()->NewInstance();
  outputData->DeepCopy(w2i->GetOutput());

  VTK_CREATE(vtkImageMapper, ia);
  ia->SetInputData(outputData);
  scalarBar->ReleaseGraphicsResources(renWin);
  sphereActor->ReleaseGraphicsResources(renWin);
  ia->SetColorWindow(255);
  ia->SetColorLevel(127.5);

  VTK_CREATE(vtkActor2D, ia2);
  ia2->SetMapper(ia);

  renWin->SetSize(320, 320);
  renWin->SetPosition(320,320);

  ren2->RemoveViewProp(scalarBar);
  ren1->RemoveViewProp(sphereActor);
  ren1->AddActor(ia2);
  renWin->RemoveRenderer(ren2);
  ren1->SetViewport(0, 0, 1, 1);

  renWin->Render();
  renWin->Render();

  vtksys::SystemTools::Delay(1000);

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  outputData->Delete();
  return !retVal;
}
