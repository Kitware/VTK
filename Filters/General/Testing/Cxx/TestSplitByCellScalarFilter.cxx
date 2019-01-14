/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSplitByCellScalarFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkSplitByCellScalarFilter.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLImageDataReader.h"

int TestSplitByCellScalarFilter(int argc, char* argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/waveletMaterial.vti");

  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName(fname);
  if (!reader->CanReadFile(fname))
  {
    std::cerr << "Error: Could not read " << fname << ".\n";
    delete [] fname;
    return EXIT_FAILURE;
  }
  reader->Update();
  delete [] fname;

  vtkImageData* image = reader->GetOutput();

  double range[2];
  image->GetCellData()->GetScalars()->GetRange(range);
  unsigned int nbMaterials = range[1] - range[0] + 1;

  // Test with image data input
  vtkNew<vtkSplitByCellScalarFilter> split;
  split->SetInputData(image);
  split->SetInputArrayToProcess(0, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS,
    vtkDataSetAttributes::SCALARS);
  split->Update();

  vtkMultiBlockDataSet* output = split->GetOutput();
  if (output->GetNumberOfBlocks() != nbMaterials)
  {
    std::cerr << "Output has " << output->GetNumberOfBlocks() <<
      " blocks instead of " << nbMaterials << std::endl;
    return EXIT_FAILURE;
  }

  // Test with unstructured grid input and pass all points option turned on
  vtkNew<vtkDataSetTriangleFilter> triangulate;
  triangulate->SetInputData(image);
  triangulate->Update();

  vtkUnstructuredGrid* grid = triangulate->GetOutput();
  split->SetInputData(grid);
  split->PassAllPointsOn();
  split->Update();

  output = split->GetOutput();
  if (output->GetNumberOfBlocks() != nbMaterials)
  {
    std::cerr << "Output has " << output->GetNumberOfBlocks() <<
      " blocks instead of " << nbMaterials << std::endl;
    return EXIT_FAILURE;
  }

  for (unsigned int i = 0; i < nbMaterials; i++)
  {
    vtkUnstructuredGrid* ug =
     vtkUnstructuredGrid::SafeDownCast(output->GetBlock(i));
    if (!ug || ug->GetNumberOfPoints() != grid->GetNumberOfPoints())
    {
      std::cerr << "Output grid " << i << " is not correct!" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test with unstructured grid input and pass all points option turned off
  split->PassAllPointsOff();
  split->Update();
  output = split->GetOutput();
  if (output->GetNumberOfBlocks() != nbMaterials)
  {
    std::cerr << "Output has " << output->GetNumberOfBlocks() <<
      " blocks instead of " << nbMaterials << std::endl;
    return EXIT_FAILURE;
  }

  for (unsigned int i = 0; i < nbMaterials; i++)
  {
    vtkUnstructuredGrid* ug =
     vtkUnstructuredGrid::SafeDownCast(output->GetBlock(i));
    if (!ug || ug->GetNumberOfPoints() == grid->GetNumberOfPoints())
    {
      std::cerr << "Output grid " << i << " is not correct!" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test with polydata input and pass all points option turned on
  vtkNew<vtkGeometryFilter> geom;
  geom->SetInputData(grid);
  geom->Update();

  vtkPolyData* mesh = geom->GetOutput();
  split->SetInputData(mesh);
  split->PassAllPointsOn();
  split->Update();
  output = split->GetOutput();
  if (output->GetNumberOfBlocks() != nbMaterials)
  {
    std::cerr << "Output has " << output->GetNumberOfBlocks() <<
      " blocks instead of " << nbMaterials << std::endl;
    return EXIT_FAILURE;
  }

  for (unsigned int i = 0; i < nbMaterials; i++)
  {
    vtkPolyData* outMesh =
     vtkPolyData::SafeDownCast(output->GetBlock(i));
    if (!outMesh || outMesh->GetNumberOfPoints() != grid->GetNumberOfPoints())
    {
      std::cerr << "Output mesh " << i << " is not correct!" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test with polydata input and pass all points option turned off
  split->PassAllPointsOff();
  split->Update();
  output = split->GetOutput();
  if (output->GetNumberOfBlocks() != nbMaterials)
  {
    std::cerr << "Output has " << output->GetNumberOfBlocks() <<
      " blocks instead of " << nbMaterials << std::endl;
    return EXIT_FAILURE;
  }

  for (unsigned int i = 0; i < nbMaterials; i++)
  {
    vtkPolyData* outMesh =
     vtkPolyData::SafeDownCast(output->GetBlock(i));
    if (!outMesh || outMesh->GetNumberOfPoints() == grid->GetNumberOfPoints())
    {
      std::cerr << "Output mesh " << i << " is not correct!" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
