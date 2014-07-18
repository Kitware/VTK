/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScalarsToColorsPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the vtkOpenGLScalarsToColorPainter to ensure that
// when the option InterpolateScalarsBeforeMapping is on,
// rendering is correct. It verifies a bug fix for 14828

#include "vtkActor.h"
#include "vtkColorTransferFunction.h"
#include "vtkCylinderSource.h"
#include "vtkDataArray.h"
#include "vtkExtractEdges.h"
#include "vtkNew.h"
#include "vtkPainterPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"

int TestScalarsToColorsPainter(int argc, char* argv[])
{
  // Set up cylinder source
  vtkNew<vtkCylinderSource> cylinder;
  cylinder->CappingOn();
  cylinder->SetResolution(32);
  cylinder->Update();

  vtkNew<vtkExtractEdges> edges;
  edges->SetInputConnection(cylinder->GetOutputPort());

  // Set up lookup table
  vtkNew<vtkColorTransferFunction> lookupTable;
  lookupTable->AddRGBPoint(0.0, 1.0, 1.0, 1.0);
  lookupTable->AddRGBPoint(1.0, 1.0, 0.0, 0.0);
  lookupTable->SetVectorModeToComponent();
  lookupTable->SetVectorComponent(0);

  // Set display to wireframe
  vtkNew<vtkPainterPolyDataMapper> mapper;
  mapper->SetInputConnection(edges->GetOutputPort());
  mapper->SetLookupTable(lookupTable.GetPointer());
  mapper->SelectColorArray("TCoords");
  mapper->SetScalarModeToUsePointFieldData();
  mapper->ScalarVisibilityOn();
  mapper->InterpolateScalarsBeforeMappingOn();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());

  // Set the wireframe color to something non-white
  vtkProperty* property = actor->GetProperty();
  property->SetRepresentationToSurface();
  property->SetAmbient(1.0);
  property->SetDiffuse(0.0);
  // Set the color to black to test that bug 14828 is fixed
  property->SetColor(0.0, 0.0, 0.0);

  // Render image
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor.GetPointer());

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  renWin->AddRenderer(renderer.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  // Compare image
  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
