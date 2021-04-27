/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOpenFOAMReaderLagrangianSerial

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenFOAMReader.h"

#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetMapper.h"
#include "vtkInformation.h"
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
static Type* findBlock(vtkMultiBlockDataSet* mb, const char* blockName)
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

int TestOpenFOAMReaderLagrangianSerial(int argc, char* argv[])
{
  // Read file name.
  char* filename = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/OpenFOAM/simplifiedSiwek-serial/simplifiedSiwek-serial.foam");

  // Read the file
  vtkNew<vtkOpenFOAMReader> reader;
  reader->SetFileName(filename);
  delete[] filename;
  reader->Update();
  reader->SetTimeValue(0.005);

  // Re-read with everything selected
  reader->EnableAllPatchArrays();
  reader->Update();
  reader->Print(std::cout);
  // reader->GetOutput()->Print(std::cout);

  auto* allBlocks = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  auto* lagrangianBlocks = findBlock<vtkMultiBlockDataSet>(allBlocks, "lagrangian");

  if (!lagrangianBlocks)
  {
    std::cout << "No lagrangian blocks!\n";
    return 1;
  }

  long nClouds = 0;
  long nParticles = 0;

  const int nLagrangianFields = reader->GetNumberOfLagrangianArrays();
  std::cout << "----- Have " << nLagrangianFields << " Lagrangian fields" << std::endl;

  // Bad name, but it is what we have
  for (int i = 0; i < reader->GetNumberOfPatchArrays(); ++i)
  {
    std::string displayName(reader->GetPatchArrayName(i));
    auto slash = displayName.rfind('/');

    if (slash != std::string::npos && displayName.compare(0, ++slash, "lagrangian/") == 0)
    {
      std::string cloudName(displayName.substr(slash));
      std::cout << "  Display " << displayName << " = Cloud <" << cloudName << ">" << std::endl;

      auto* cloudData = findBlock<vtkPolyData>(lagrangianBlocks, cloudName.c_str());
      if (cloudData)
      {
        ++nClouds;
        nParticles += cloudData->GetNumberOfPoints();
      }
    }
  }
  std::cout << "  Read " << nParticles << " particles from " << nClouds << " clouds" << std::endl;

  int retVal = (nParticles != 0);

  return !retVal;
}
