/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSimplePointsReaderWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenFOAMReader.h"

#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkUnstructuredGrid.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

namespace
{

// Get named block of specified type
template <class Type>
static Type* findBlock(vtkMultiBlockDataSet* mb, const char* blockName)
{
  const unsigned int nblocks = mb->GetNumberOfBlocks();
  std::cerr << "has " << nblocks << " blocks\n";
  for (unsigned int blocki = 0; blocki < nblocks; ++blocki)
  {
    vtkDataObject* obj = mb->GetBlock(blocki);
    std::cerr << "check block: " << mb->GetMetaData(blocki)->Get(vtkCompositeDataSet::NAME())
              << std::endl;

    auto* subblock = vtkMultiBlockDataSet::SafeDownCast(obj);

    Type* dataset;
    if (subblock)
    {
      dataset = findBlock<Type>(subblock, blockName);
    }
    else if (strcmp(mb->GetMetaData(blocki)->Get(vtkCompositeDataSet::NAME()), blockName) == 0)
    {
      dataset = Type::SafeDownCast(obj);
    }
    if (dataset)
    {
      return dataset;
    }
  }
  return nullptr;
}

} // End anonymous namespace

int TestOpenFOAMReaderValuePointPatch(int argc, char* argv[])
{
  // Read file name.
  char* filename = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/OpenFOAM/valuePointPatch/valuePointPatch.foam");

  // Read the file
  vtkNew<vtkOpenFOAMReader> reader;
  reader->SetFileName(filename);
  delete[] filename;
  reader->Update();

  reader->SetTimeValue(2002);
  reader->EnableAllPatchArrays();
  reader->Update();

  auto* allBlocks = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  if (!allBlocks)
  {
    std::cout << "No blocks!\n";
    return 1;
  }

  auto* pointPatch = findBlock<vtkPolyData>(allBlocks, "visor");
  if (!pointPatch)
  {
    std::cout << "No point patch!\n";
    return 1;
  }

  pointPatch->GetPointData()->SetScalars(pointPatch->GetPointData()->GetArray("pointLocations"));

  // Visualize
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(pointPatch);
  mapper->ScalarVisibilityOn();
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarRange(1.1, 1.3);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->SetBackground(0, 0, 0);

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return EXIT_SUCCESS;
}
