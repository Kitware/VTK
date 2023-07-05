// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// When a globalFaceZone is used in foam-extend in parallel (e.g. GGI), the
// owner list will be shorter than the face list. This test ensures the correct
// behavior in that case and also checks reading of the global face zone itself.

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "vtkPOpenFOAMReader.h"

#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

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

int TestPOpenFOAMReaderGlobalFaceZone(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> controller;
#else
  vtkNew<vtkDummyController> controller;
#endif

  controller->Initialize(&argc, &argv);
  int rank = controller->GetLocalProcessId();
  vtkLogger::SetThreadName("rank=" + std::to_string(rank));
  vtkMultiProcessController::SetGlobalController(controller);

  // Read file name
  char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/OpenFOAM/mixerGgi/mixerGgi.foam");

  // Read the file
  vtkNew<vtkPOpenFOAMReader> reader;
  reader->SetFileName(filename);
  delete[] filename;
  reader->SetCaseType(vtkPOpenFOAMReader::DECOMPOSED_CASE);
  reader->ReadZonesOn();
  reader->CopyDataToCellZonesOn();
  reader->Update();

  reader->SetTimeValue(0.5);

  // Re-read with everything selected
  reader->EnableAllPatchArrays();
  reader->Update();
  reader->Print(std::cout);
  reader->GetOutput()->Print(std::cout);

  auto* allBlocks = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());

  std::cout << "Read " << allBlocks->GetNumberOfBlocks() << " blocks" << std::endl;

  auto* zoneBlocks = findBlock<vtkMultiBlockDataSet>(allBlocks, "zones");

  if (!zoneBlocks)
  {
    std::cout << "No zone blocks!\n";
    return 1;
  }

  // Get the first polyData set (faces)
  auto* fZone = findBlock<vtkPolyData>(zoneBlocks);
  if (!fZone)
  {
    std::cout << "No faceZone!\n";
    return 1;
  }

  fZone->GetCellData()->SetScalars(fZone->GetCellData()->GetArray("p"));

  controller->Finalize();

  return 0;
}
