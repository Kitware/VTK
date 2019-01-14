/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCategoricalMultiBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can give each block its own material and
// also override them easily.
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
#include "vtkCompositePolyDataMapper2.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkLookupTable.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkOSPRayMaterialLibrary.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOSPRayTestInteractor.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

int TestCategoricalMultiBlock(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // set up the environment
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetSize(700,700);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  renderer->SetBackground(0.0,0.0,0.0);
  renderer->SetBackground2(0.8,0.8,1.0);
  renderer->GradientBackgroundOn();
  renWin->AddRenderer(renderer);
  vtkSmartPointer<vtkOSPRayPass> ospray = vtkSmartPointer<vtkOSPRayPass>::New();
  renderer->SetPass(ospray);
  vtkOSPRayRendererNode::SetRendererType("pathtracer", renderer);
  vtkSmartPointer<vtkOSPRayTestInteractor> style =
    vtkSmartPointer<vtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  //make some predictable data to test with
  vtkSmartPointer<vtkMultiBlockDataSet> mbds = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  mbds->SetNumberOfBlocks(12);
  for (int i = 0; i < 12; i++)
  {
    vtkSmartPointer<vtkSphereSource> polysource = vtkSmartPointer<vtkSphereSource>::New();
    polysource->SetPhiResolution(10);
    polysource->SetThetaResolution(10);
    polysource->SetCenter(i%4, i/4, 0);
    polysource->Update();

    vtkPolyData *pd = polysource->GetOutput();
    vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
    da->SetNumberOfComponents(1);
    da->SetName("test array");
    for (int c = 0; c < pd->GetNumberOfCells(); c++)
    {
      da->InsertNextValue(i);
    }
    pd->GetCellData()->SetScalars(da);

    mbds->SetBlock(i, polysource->GetOutput());
  }

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

  //todo: test should turn on/off or let user do so
  lut->SetIndexedLookup(1);

  //get a hold of the material library
  vtkSmartPointer<vtkOSPRayMaterialLibrary> ml = vtkSmartPointer<vtkOSPRayMaterialLibrary>::New();
  vtkOSPRayRendererNode::SetMaterialLibrary(ml, renderer);
  //add materials to it
  ml->AddMaterial("Five", "Metal");
  ml->AddMaterial("One", "Glass");
  //some of material names use the same low level material implementation
  ml->AddMaterial("Two", "Glass");
  //but each one  can be tuned
  double green[3] = {0.0,0.9,0.0};
  ml->AddShaderVariable("Two", "attenuationColor", 3, green);
  ml->AddShaderVariable("Two", "eta", {1.});
  ml->AddMaterial("Three", "Glass");
  double blue[3] = {0.0,0.0,0.9};
  ml->AddShaderVariable("Three", "attenuationColor", 3, blue);
  ml->AddShaderVariable("Three", "eta", {1.65});

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  vtkProperty *prop;
  prop = actor->GetProperty();
  prop->SetMaterialName("Value Indexed"); //making submaterials

  vtkSmartPointer<vtkCompositePolyDataMapper2> mapper=vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  mapper->SetInputDataObject(mbds);
  mapper->SetLookupTable(lut);
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  //override one of the block's with a different material
  vtkSmartPointer<vtkCompositeDataDisplayAttributes> cda =
    vtkSmartPointer<vtkCompositeDataDisplayAttributes>::New();
  mapper->SetCompositeDataDisplayAttributes(cda);

  unsigned int top_index = 0;
  auto dobj = cda->DataObjectFromIndex(12, mbds, top_index);
  cda->SetBlockMaterial(dobj, "Five");

  //set up progressive rendering
  vtkCommand *looper = style->GetLooper(renWin);
  vtkCamera *cam = renderer->GetActiveCamera();
  iren->AddObserver(vtkCommand::KeyPressEvent, looper);
  cam->AddObserver(vtkCommand::ModifiedEvent, looper);
  iren->CreateRepeatingTimer(10); //every 10 msec we'll rerender if needed
  iren->AddObserver(vtkCommand::TimerEvent, looper);

  //todo: use standard vtk testing conventions
  iren->Start();
  return 0;
}
