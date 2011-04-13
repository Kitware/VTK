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
// .NAME Test of vtkSimplePointsReader and vtkSimplePointsWriter
// .SECTION Description
//

#include "vtkSimplePointsReader.h"
#include "vtkSimplePointsWriter.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

int TestSimplePointsReaderWriter( int vtkNotUsed(argc), char *vtkNotUsed(argv)[] )
{
  // Create a sphere.
  vtkSmartPointer<vtkSphereSource> sphereSource = 
    vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->Update();

  // Write the data.
  vtkSmartPointer<vtkSimplePointsWriter> writer =
    vtkSmartPointer<vtkSimplePointsWriter>::New();
  writer->SetInputConnection(sphereSource->GetOutputPort());
  writer->SetFileName("SimplePoints.xyz");
  writer->Write();

  // Create the reader.
  vtkSmartPointer<vtkSimplePointsReader> reader =
    vtkSmartPointer<vtkSimplePointsReader>::New();
  reader->SetFileName("SimplePoints.xyz");
  reader->Update();

  if(reader->GetOutput()->GetNumberOfPoints() != sphereSource->GetOutput()->GetNumberOfPoints())
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
