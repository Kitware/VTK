// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkGenericCell.h>
#include <vtkMathUtilities.h>
#include <vtkNew.h>
#include <vtkPolyhedron.h>
#include <vtkTestUtilities.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridReader.h>

int TestPolyhedronConcaveCentroid(int argc, char* argv[])
{
  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/concavePolyhedron.vtu");

  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  delete[] filename;

  vtkNew<vtkGenericCell> genericCell;
  reader->GetOutput()->GetCell(0, genericCell);
  vtkPolyhedron* polyhedron = vtkPolyhedron::SafeDownCast(genericCell->GetRepresentativeCell());
  // call these to ensure the polyhedron is initialized
  std::cout << "Number Of Points: " << polyhedron->GetNumberOfPoints() << std::endl;
  std::cout << "Number Of Faces: " << polyhedron->GetNumberOfFaces() << std::endl;
  double centroid[3];
  if (!polyhedron->GetCentroid(centroid))
  {
    std::cerr << "Failed to compute centroid." << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << std::setprecision(12) << "Centroid: (" << centroid[0] << ", " << centroid[1] << ", "
            << centroid[2] << ")" << std::endl;
  if (vtkMathUtilities::FuzzyCompare(centroid[0], 1.40909090909, 1e-11) &&
    vtkMathUtilities::FuzzyCompare(centroid[1], 2.40909090909, 1e-11) &&
    vtkMathUtilities::FuzzyCompare(centroid[2], 0.5))
  {
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}
