/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPOpenFOAMReaderLagrangianSerial

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "vtkPOpenFOAMReader.h"

#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetMapper.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

namespace
{

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

int TestPOpenFOAMReaderLagrangianSerial(int argc, char* argv[])
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

  // Read file name.
  char* filename = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/OpenFOAM/simplifiedSiwek-serial/simplifiedSiwek-serial.foam");

  // Read the file
  vtkNew<vtkPOpenFOAMReader> reader;
  reader->SetFileName(filename);
  delete[] filename;
  reader->SetCaseType(vtkPOpenFOAMReader::RECONSTRUCTED_CASE);
  reader->Update();

  reader->SetTimeValue(0.005);

  // Re-read with everything selected
  reader->EnableAllPatchArrays();
  reader->Update();
  reader->Print(std::cout);
  // reader->GetOutput()->Print(std::cout);

  long nClouds = 0;
  long nParticles = 0;
  int hasLagrangian = 0;

  do
  {
    auto* allBlocks = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
    if (!allBlocks)
    {
      std::cout << "No blocks!\n";
      break;
    }
    std::cout << "Read " << allBlocks->GetNumberOfBlocks() << " blocks" << std::endl;

    auto* lagrangianBlocks = findBlock<vtkMultiBlockDataSet>(allBlocks, "lagrangian");
    if (!lagrangianBlocks)
    {
      std::cout << "No lagrangian blocks!\n";
      break;
    }

    // Bad name, but it is what we have
    for (int i = 0; i < reader->GetNumberOfPatchArrays(); ++i)
    {
      std::string displayName(reader->GetPatchArrayName(i));
      auto slash = displayName.rfind('/');

      if (slash != std::string::npos && displayName.compare(0, ++slash, "lagrangian/") == 0)
      {
        hasLagrangian = 1;
        std::string cloudName(displayName.substr(slash));
        std::cout << "  Display " << displayName << " = Cloud <" << cloudName << ">" << std::endl;

        auto* cloudData = findBlock<vtkPolyData>(lagrangianBlocks, cloudName.c_str());
        if (cloudData)
        {
          ++nClouds;
          nParticles += static_cast<long>(cloudData->GetNumberOfPoints());
        }
      }
    }
  } while (false);

  int globalHasLagrangian = hasLagrangian;
  long nGlobalClouds = nClouds;
  long nGlobalParticles = nParticles;

  // maxOp for clouds may be misleading, but good enough for test
  controller->AllReduce(&hasLagrangian, &globalHasLagrangian, 1, vtkCommunicator::LOGICAL_OR_OP);
  controller->AllReduce(&nClouds, &nGlobalClouds, 1, vtkCommunicator::MAX_OP);
  controller->AllReduce(&nParticles, &nGlobalParticles, 1, vtkCommunicator::SUM_OP);

  int retVal = 0;
  if (rank == 0)
  {
    std::cout << "  Read " << nParticles << " particles from " << nClouds << " clouds" << std::endl;
    retVal = (nParticles != 0);
  }
  controller->Barrier();
  controller->Broadcast(&retVal, 1, 0);

  controller->Finalize();

  return !retVal;
}
