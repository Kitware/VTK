/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCategoricalMaterials.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can assign materials to individual cells.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
//              In interactive mode it responds to the keys listed
//              vtkOSPRayTestInteractor.h

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkColorSeries.h"
#include "vtkDoubleArray.h"
#include "vtkLookupTable.h"
#include "vtkOSPRayMaterialLibrary.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOSPRayTestInteractor.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

int TestCategoricalMaterials(int argc, char* argv[])
{
  // set up the environment
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetSize(700, 700);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkOSPRayRendererNode::SetBackgroundMode(2, renderer);
  renderer->SetEnvironmentalBG(0.0, 0.0, 0.0);
  renderer->SetEnvironmentalBG2(0.8, 0.8, 1.0);
  renderer->GradientEnvironmentalBGOn();
  renWin->AddRenderer(renderer);
  vtkSmartPointer<vtkOSPRayPass> ospray = vtkSmartPointer<vtkOSPRayPass>::New();
  renderer->SetPass(ospray);
  vtkOSPRayRendererNode::SetRendererType("pathtracer", renderer);
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      vtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      break;
    }
  }

  vtkSmartPointer<vtkOSPRayTestInteractor> style = vtkSmartPointer<vtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  // make some predictable data to test with
  vtkSmartPointer<vtkPlaneSource> polysource = vtkSmartPointer<vtkPlaneSource>::New();
  polysource->SetXResolution(4);
  polysource->SetYResolution(3);
  polysource->Update();
  vtkPolyData* pd = polysource->GetOutput();
  vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
  da->SetNumberOfComponents(1);
  da->SetName("test array");
  for (int i = 0; i < pd->GetNumberOfCells(); i++)
  {
    da->InsertNextValue(i);
  }
  pd->GetCellData()->SetScalars(da); // this is what we'll color by, including materials

  // Choose a color scheme
  vtkSmartPointer<vtkColorSeries> palettes = vtkSmartPointer<vtkColorSeries>::New();
  palettes->SetColorSchemeByName("Brewer Qualitative Set3");
  // Create the LUT and add some annotations.
  vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
  lut->SetAnnotation(0., "Zero");
  lut->SetAnnotation(1., "One");
  lut->SetAnnotation(2., "Two");
  lut->SetAnnotation(3, "Three");
  lut->SetAnnotation(4, "Four");
  lut->SetAnnotation(5, "Five");
  lut->SetAnnotation(6, "Six");
  lut->SetAnnotation(7, "Seven");
  lut->SetAnnotation(8, "Eight");
  lut->SetAnnotation(9, "Nine");
  lut->SetAnnotation(10, "Ten");
  lut->SetAnnotation(11, "Eleven");
  lut->SetAnnotation(12, "Twelve");
  palettes->BuildLookupTable(lut);

  // todo: test should turn on/off or let user do so
  lut->SetIndexedLookup(1);

  // get a hold of the material library
  vtkSmartPointer<vtkOSPRayMaterialLibrary> ml = vtkSmartPointer<vtkOSPRayMaterialLibrary>::New();
  vtkOSPRayRendererNode::SetMaterialLibrary(ml, renderer);
  // add materials to it
  ml->AddMaterial("Four", "Metal");
  ml->AddMaterial("One", "ThinGlass");
  // some of material names use the same low level material implementation
  ml->AddMaterial("Two", "ThinGlass");
  // but each one  can be tuned
  double green[3] = { 0.0, 0.9, 0.0 };
  ml->AddShaderVariable("Two", "attenuationColor", 3, green);
  ml->AddShaderVariable("Two", "eta", { 1. });
  ml->AddMaterial("Three", "ThinGlass");
  double blue[3] = { 0.0, 0.0, 0.9 };
  ml->AddShaderVariable("Three", "attenuationColor", 3, blue);
  ml->AddShaderVariable("Three", "eta", { 1.65 });

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  vtkProperty* prop;
  prop = actor->GetProperty();
  prop->SetMaterialName("Value Indexed"); // using several from the library

  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(pd);
  mapper->SetLookupTable(lut);
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  // set up progressive rendering
  vtkCommand* looper = style->GetLooper(renWin);
  vtkCamera* cam = renderer->GetActiveCamera();
  iren->AddObserver(vtkCommand::KeyPressEvent, looper);
  cam->AddObserver(vtkCommand::ModifiedEvent, looper);
  iren->CreateRepeatingTimer(10); // every 10 msec we'll rerender if needed
  iren->AddObserver(vtkCommand::TimerEvent, looper);

  // todo: use standard vtk testing conventions
  iren->Start();
  return 0;
}
