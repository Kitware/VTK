/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNamedColorsIntegration.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkNamedColors.h>

#include <vtkConeSource.h>
#include <vtkAlgorithm.h>
#include <vtkElevationFilter.h>
#include <vtkBandedPolyDataContourFilter.h>
#include <vtkLookupTable.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkOpenGLPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include "vtkRegressionTestImage.h"

//#include <iostream>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

// Create a cone, contour it using the banded contour filter and
// color it with the primary additive and subtractive colors.
int TestNamedColorsIntegration(int argc, char* argv[])
{
  VTK_CREATE(vtkNamedColors, namedColors);
  //namedColors->PrintSelf(std::cout,vtkIndent(2));

  // Create a cone
  VTK_CREATE(vtkConeSource, coneSource);
  coneSource->SetCenter(0.0, 0.0, 0.0);
  coneSource->SetRadius(5.0);
  coneSource->SetHeight(10);
  coneSource->SetDirection(0,1,0);
  coneSource->Update();

  double bounds[6];
  coneSource->GetOutput()->GetBounds(bounds);

  VTK_CREATE(vtkElevationFilter, elevation);
  elevation->SetInputConnection(coneSource->GetOutputPort());
  elevation->SetLowPoint(0,bounds[2],0);
  elevation->SetHighPoint(0,bounds[3],0);

  VTK_CREATE(vtkBandedPolyDataContourFilter, bcf);
  bcf->SetInputConnection(elevation->GetOutputPort());
  bcf->SetScalarModeToValue();
  bcf->GenerateContourEdgesOn();
  bcf->GenerateValues(7,elevation->GetScalarRange());

  // Build a simple lookup table of
  // primary additive and subtractive colors.
  VTK_CREATE(vtkLookupTable, lut);
  lut->SetNumberOfTableValues(7);
  double rgba[4];
  // Test setting and getting colors here.
  namedColors->GetColor("Red",rgba);
  namedColors->SetColor("My Red",rgba);
  namedColors->GetColor("My Red",rgba);
  lut->SetTableValue(0,rgba);
  namedColors->GetColor("DarkGreen",rgba);
  lut->SetTableValue(1,rgba);
  namedColors->GetColor("Blue",rgba);
  lut->SetTableValue(2,rgba);
  namedColors->GetColor("Cyan",rgba);
  lut->SetTableValue(3,rgba);
  namedColors->GetColor("Magenta",rgba);
  lut->SetTableValue(4,rgba);
  namedColors->GetColor("Yellow",rgba);
  lut->SetTableValue(5,rgba);
  namedColors->GetColor("White",rgba);
  lut->SetTableValue(6,rgba);
  lut->SetTableRange(elevation->GetScalarRange());
  lut->Build();

  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(bcf->GetOutputPort());
  mapper->SetScalarRange(elevation->GetScalarRange());
  mapper->SetLookupTable(lut);
  mapper->SetScalarModeToUseCellData();

  VTK_CREATE(vtkPolyDataMapper, contourLineMapper );
  contourLineMapper->SetInputData(bcf->GetContourEdgesOutput());
  contourLineMapper->SetScalarRange(elevation->GetScalarRange());
  contourLineMapper->SetResolveCoincidentTopologyToPolygonOffset();

  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);

  VTK_CREATE(vtkActor, contourLineActor);
  contourLineActor->SetMapper(contourLineMapper);
  contourLineActor->GetProperty()->SetColor(namedColors->GetColorAsDoubleRGB("black"));

  VTK_CREATE(vtkRenderer, renderer);
  VTK_CREATE(vtkRenderWindow, renderWindow);
  renderWindow->AddRenderer(renderer);
  VTK_CREATE(vtkRenderWindowInteractor, renderWindowInteractor);
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->AddActor(contourLineActor);
  renderer->SetBackground(namedColors->GetColorAsDoubleRGB("SteelBlue"));

  renderWindow->Render();

  int retVal = vtkRegressionTestImage( renderWindow.GetPointer() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    renderWindowInteractor->Start();
    }

  return EXIT_SUCCESS;
}
