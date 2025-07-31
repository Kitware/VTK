// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkIntArray.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkSmartPointer.h>
#include <vtkSurfaceNets3D.h>

#include <cassert>
#include <iostream>

int TestSurfaceNets3DNormalsConsistency(int, char*[])
{
  // Create labeled image
  vtkNew<vtkImageData> image;
  image->SetDimensions(50, 50, 50);
  image->AllocateScalars(VTK_INT, 1);

  // Fill with zero
  std::fill_n(static_cast<int*>(image->GetScalarPointer()), 50 * 50 * 50, 0);

  // Add 4 adjacent cubes (20x20x20) with values 1-4 around the center edge (25,25,:)
  for (int z = 15; z < 35; ++z)
  {
    for (int y = 15; y < 35; ++y)
    {
      for (int x = 0; x < 50; ++x)
      {
        int value = 0;
        if (x >= 15 && x < 35)
        {
          if (x < 25 && y < 25)
          {
            value = 1;
          }
          else if (x >= 25 && y < 25)
          {
            value = 2;
          }
          else if (x < 25)
          {
            value = 3;
          }
          else
          {
            value = 4;
          }
        }
        if (value > 0)
        {
          *static_cast<int*>(image->GetScalarPointer(x, y, z)) = value;
        }
      }
    }
  }

  // Run vtkSurfaceNets3D
  vtkNew<vtkSurfaceNets3D> surfacenets;
  surfacenets->SetBackgroundLabel(0);
  surfacenets->SetInputData(image);
  surfacenets->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ImageScalars");
  surfacenets->SetOutputMeshTypeToTriangles();
  surfacenets->Update();

  vtkPolyData* surface = surfacenets->GetOutput();

  // Run vtkPolyDataNormals
  vtkNew<vtkPolyDataNormals> normalsFilter_original;
  normalsFilter_original->SetInputData(surface);
  normalsFilter_original->ComputeCellNormalsOn();
  normalsFilter_original->ComputePointNormalsOff();
  normalsFilter_original->AutoOrientNormalsOff(); // Don't change orientation
  normalsFilter_original->Update();

  // Run vtkPolyDataNormals
  vtkNew<vtkPolyDataNormals> normalsFilter;
  normalsFilter->SetInputData(surface);
  normalsFilter->ComputeCellNormalsOn();
  normalsFilter->ComputePointNormalsOff();
  normalsFilter->AutoOrientNormalsOn(); // Ensure consistent normal orientation
  normalsFilter->Update();

  // Check normals are consistent
  vtkDataArray* normalsBefore = normalsFilter_original->GetOutput()->GetCellData()->GetNormals();
  vtkDataArray* normalsAfter = normalsFilter->GetOutput()->GetCellData()->GetNormals();

  if (!normalsBefore || !normalsAfter)
  {
    std::cerr << "Normals not present before or after vtkPolyDataNormals.\n";
    return EXIT_FAILURE;
  }

  if (normalsBefore->GetNumberOfTuples() != normalsAfter->GetNumberOfTuples())
  {
    std::cerr << "Number of normals mismatch.\n";
    return EXIT_FAILURE;
  }

  for (vtkIdType i = 0; i < normalsBefore->GetNumberOfTuples(); ++i)
  {
    double n0[3], n1[3];
    normalsBefore->GetTuple(i, n0);
    normalsAfter->GetTuple(i, n1);
    double dot = vtkMath::Dot(n0, n1);
    if (dot < 0.99)
    { // Allow for small deviation
      std::cerr << "Normal mismatch at point " << i << ": dot = " << dot << "\n";
      return EXIT_FAILURE;
    }
  }

  // Check BoundaryLabels CellData array
  vtkIntArray* labels =
    vtkIntArray::SafeDownCast(surface->GetCellData()->GetArray("BoundaryLabels"));
  if (!labels)
  {
    std::cerr << "BoundaryLabels array missing from CellData.\n";
    return EXIT_FAILURE;
  }

  if (labels->GetNumberOfComponents() != 2)
  {
    std::cerr << "BoundaryLabels does not have 2 components.\n";
    return EXIT_FAILURE;
  }

  for (vtkIdType i = 0; i < labels->GetNumberOfTuples(); ++i)
  {
    int v[2];
    labels->GetTypedTuple(i, v);
    if (v[0] == 0)
    {
      std::cerr << "BoundaryLabels at cell " << i << " background must be second component: ["
                << v[0] << ", " << v[1] << "]\n";
      return EXIT_FAILURE;
    }
    if (v[0] >= v[1] && v[1] != 0)
    {
      std::cerr << "BoundaryLabels at cell " << i << " are not sorted: [" << v[0] << ", " << v[1]
                << "]\n";
      return EXIT_FAILURE;
    }
  }

  std::cout << "Test passed: Surface normals consistent and BoundaryLabels correctly formatted.\n";
  return EXIT_SUCCESS;
}
