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
#include "vtkInformation.h"
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

} // End anonymous namespace

int TestOpenFOAMReaderLargePolyhedral(int argc, char* argv[])
{
  // Read file name.
  char* filename = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/OpenFOAM/largePolyhedral/largePolyhedral.foam");

  // Read the file
  vtkNew<vtkOpenFOAMReader> reader;
  reader->SetFileName(filename);
  delete[] filename;
  reader->Update();

  // Read everything
  reader->EnableAllPatchArrays();
  reader->Update();

  reader->Print(std::cout);
  reader->GetOutput()->Print(std::cout);

  auto* allBlocks = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  if (!allBlocks)
  {
    std::cout << "No blocks!\n";
    return 1;
  }

  vtkIdType nCells = 0;

  auto* ug = findBlock<vtkUnstructuredGrid>(allBlocks);
  if (ug)
  {
    nCells = ug->GetNumberOfCells();
  }

  int retVal = (nCells != 1);

  return retVal;
}
