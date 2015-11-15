/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStaticCellLinks.h"
#include "vtkSmartPointer.h"
#include "vtkImageData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPolyData.h"
#include "vtkExtractGeometry.h"
#include "vtkSphere.h"
#include "vtkTimerLog.h"

int TestStaticCellLinks( int, char *[] )
{
  int dataDim = 3;

  vtkSmartPointer<vtkImageData> volume =
    vtkSmartPointer<vtkImageData>::New();
  volume->SetDimensions(dataDim,dataDim,dataDim);
  volume->AllocateScalars(VTK_INT,1);

  vtkSmartPointer<vtkSphere> sphere =
    vtkSmartPointer<vtkSphere>::New();
  sphere->SetCenter(0,0,0);
  sphere->SetRadius(100000);

  // Side effect of this filter is conversion of volume to unstructured grid
  vtkSmartPointer<vtkExtractGeometry> extract =
    vtkSmartPointer<vtkExtractGeometry>::New();
  extract->SetInputData(volume);
  extract->SetImplicitFunction(sphere);
  extract->Update();

  // Grab the output
  vtkSmartPointer<vtkUnstructuredGrid> ugrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  ugrid = extract->GetOutput();

  vtkStaticCellLinks<int> slinks;
  slinks.BuildLinks(ugrid);

  int numCells = slinks.GetNumberOfCells(0);
  const int *cells = slinks.GetCells(0);
  cout << "Lower Left corner (numCells, cells): " << numCells << " (";
  for (int i=0; i<numCells; ++i)
    {
    cout << cells[i];
    if ( i < (numCells-1) ) cout << "," ;
    }
  cout << ")\n";
  if ( numCells != 1 || cells[0] != 0 )
    {
    return EXIT_FAILURE;
    }


  numCells = slinks.GetNumberOfCells(13);
  cells = slinks.GetCells(13);
  cout << "Center (numCells, cells): " << numCells << " (";
  for (int i=0; i<numCells; ++i)
    {
    cout << cells[i];
    if ( i < (numCells-1) ) cout << "," ;
    }
  cout << ")\n";
  if ( numCells != 8 )
    {
    return EXIT_FAILURE;
    }

  numCells = slinks.GetNumberOfCells(26);
  cells = slinks.GetCells(26);
  cout << "Upper Right corner (numCells, cells): " << numCells << " (";
  for (int i=0; i<numCells; ++i)
    {
    cout << cells[i];
    if ( i < (numCells-1) ) cout << "," ;
    }
  cout << ")\n";
  if ( numCells != 1 || cells[0] != 7 )
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
