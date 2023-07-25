// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkSimplePointsReader and vtkSimplePointsWriter
// .SECTION Description
//

#include "vtkSimplePointsReader.h"
#include "vtkSimplePointsWriter.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

int TestSimplePointsReaderWriter(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create a sphere.
  vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->Update();

  // Write the data.
  vtkSmartPointer<vtkSimplePointsWriter> writer = vtkSmartPointer<vtkSimplePointsWriter>::New();
  writer->SetInputConnection(sphereSource->GetOutputPort());
  writer->SetFileName("SimplePoints.xyz");
  writer->Write();

  // Create the reader.
  vtkSmartPointer<vtkSimplePointsReader> reader = vtkSmartPointer<vtkSimplePointsReader>::New();
  reader->SetFileName("SimplePoints.xyz");
  reader->Update();

  if (reader->GetOutput()->GetNumberOfPoints() != sphereSource->GetOutput()->GetNumberOfPoints())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
