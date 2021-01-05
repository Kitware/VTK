/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOpenFOAMReaderDimensionedFields.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenFOAMReader.h"

#include "vtkCellData.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkUnstructuredGrid.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

int TestOpenFOAMReaderDimensionedFields(int argc, char* argv[])
{
  // Read file name.
  char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/OpenFOAM/DimFields/cavity/cavity.foam");

  // Read the file
  vtkNew<vtkOpenFOAMReader> reader;
  reader->SetFileName(filename);
  delete[] filename;

  // Visualize
  vtkNew<vtkCompositeDataGeometryFilter> polyFilter;
  polyFilter->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(polyFilter->GetOutputPort());
  mapper->SetScalarRange(1, 2);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->SetBackground(.2, .4, .6);

  renderWindow->Render();

  // TODO: regression image
  // int retVal = vtkRegressionTestImage(renderWindow);
  // if (retVal == vtkRegressionTester::DO_INTERACTOR)
  // {
  //   renderWindowInteractor->Start();
  // }

  return EXIT_SUCCESS;
}
