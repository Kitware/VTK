/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScalarsToColorsPainterFieldData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests coloring by a field data

#include "vtkActor.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkPainterPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkStripper.h"
#include "vtkTestUtilities.h"

int TestScalarsToColorsPainterFieldData(int argc, char* argv[])
{
  vtkIdType tupleId = -1;
  if (argc > 1)
    {
    tupleId = atoi(argv[1]);
    }

  // Set up sphere source
  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(8);
  sphere->SetThetaResolution(8);
  sphere->Update();

  // Add a field data array
  vtkPolyData* pd = sphere->GetOutput();
  vtkFieldData* fd = pd->GetFieldData();

  vtkNew<vtkFloatArray> newArray;
  newArray->SetName("floatArray");
  newArray->SetNumberOfComponents(1);
  newArray->SetNumberOfTuples(pd->GetNumberOfCells());
  for (vtkIdType i = 0; i < pd->GetNumberOfCells(); ++i)
    {
    float value = static_cast<float>(i);
    newArray->SetTuple(i, &value);
    }

  fd->AddArray(newArray.GetPointer());

  // Set up lookup table
  vtkNew<vtkColorTransferFunction> lookupTable;
  lookupTable->AddRGBPoint(0.0, 1.0, 1.0, 1.0);
  lookupTable->AddRGBPoint(static_cast<float>(pd->GetNumberOfCells()), 1.0, 0.0, 0.0);

  vtkNew<vtkPainterPolyDataMapper> mapper;
  mapper->SetInputData(pd);
  mapper->SetLookupTable(lookupTable.GetPointer());
  mapper->SelectColorArray("floatArray");
  mapper->SetScalarModeToUseFieldData();
  mapper->SetFieldDataTupleId(tupleId);
  mapper->ScalarVisibilityOn();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());

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
