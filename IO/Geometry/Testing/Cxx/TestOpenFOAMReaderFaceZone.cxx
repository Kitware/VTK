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

// Get any block of specified type
template <class Type>
Type* findBlock(vtkMultiBlockDataSet* mb)
{
  Type* dataset = nullptr;
  const unsigned int nblocks = (mb ? mb->GetNumberOfBlocks() : 0u);
  for (unsigned int blocki = 0; !dataset && blocki < nblocks; ++blocki)
  {
    vtkDataObject* obj = mb->GetBlock(blocki);
    dataset = Type::SafeDownCast(obj);
    if (!dataset)
    {
      dataset = findBlock<Type>(vtkMultiBlockDataSet::SafeDownCast(obj));
    }
  }
  return dataset;
}

// Get named block of specified type
template <class Type>
Type* findBlock(vtkMultiBlockDataSet* mb, const char* blockName)
{
  Type* dataset = nullptr;
  const unsigned int nblocks = (mb ? mb->GetNumberOfBlocks() : 0u);
  for (unsigned int blocki = 0; !dataset && blocki < nblocks; ++blocki)
  {
    vtkDataObject* obj = mb->GetBlock(blocki);
    if (strcmp(mb->GetMetaData(blocki)->Get(vtkCompositeDataSet::NAME()), blockName) == 0)
    {
      dataset = Type::SafeDownCast(obj);
    }
    if (!dataset)
    {
      dataset = findBlock<Type>(vtkMultiBlockDataSet::SafeDownCast(obj), blockName);
    }
  }
  return dataset;
}

} // End anonymous namespace

int TestOpenFOAMReaderFaceZone(int argc, char* argv[])
{
  // Read file name.
  char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/OpenFOAM/squareBend/squareBend.foam");

  // Read the file
  vtkNew<vtkOpenFOAMReader> reader;
  reader->SetFileName(filename);
  delete[] filename;

  reader->SetTimeValue(100);
  // These method names will be revised in the future
  reader->ReadZonesOn();
  reader->CopyDataToCellZonesOn();

  // Dont currently need anything selected (controlled above)
  reader->DisableAllPatchArrays();
  reader->Update();

  auto* allBlocks = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  auto* zoneBlocks = findBlock<vtkMultiBlockDataSet>(allBlocks, "zones");

  if (!zoneBlocks)
  {
    std::cout << "No zone blocks!\n";
    return 1;
  }

  // Get the first polyData set (faces)
  auto* fzone = findBlock<vtkPolyData>(zoneBlocks);
  if (!fzone)
  {
    std::cout << "No faceZone!\n";
    return 1;
  }

  fzone->GetCellData()->SetScalars(fzone->GetCellData()->GetArray("p"));

  // Visualize
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(fzone);
  mapper->ScalarVisibilityOn();
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarRange(-40, 80);

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
