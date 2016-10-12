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
#include "vtkStaticCellLinksTemplate.h"
#include "vtkSmartPointer.h"
#include "vtkImageData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPolyData.h"
#include "vtkExtractGeometry.h"
#include "vtkSphere.h"
#include "vtkSphereSource.h"
#include "vtkTimerLog.h"

// Test the building of static cell links in both unstructured and structured
// grids.
int TestStaticCellLinks( int, char *[] )
{
  int dataDim = 3;

  // First create a volume which will be converted to an unstructured grid
  vtkSmartPointer<vtkImageData> volume =
    vtkSmartPointer<vtkImageData>::New();
  volume->SetDimensions(dataDim,dataDim,dataDim);
  volume->AllocateScalars(VTK_INT,1);

  //----------------------------------------------------------------------------
  // Build links on volume
  vtkSmartPointer<vtkStaticCellLinks> imlinks =
    vtkSmartPointer<vtkStaticCellLinks>::New();
  imlinks->BuildLinks(volume);

  vtkIdType ncells = imlinks->GetNumberOfCells(0);
  const vtkIdType *imcells = imlinks->GetCells(0);
  cout << "Volume:\n";
  cout << "   Lower Left corner (numCells, cells): " << ncells << " (";
  for (int i=0; i<ncells; ++i)
  {
    cout << imcells[i];
    if ( i < (ncells-1) ) cout << "," ;
  }
  cout << ")\n";
  if ( ncells != 1 || imcells[0] != 0 )
  {
    return EXIT_FAILURE;
  }

  ncells = imlinks->GetNumberOfCells(13);
  imcells = imlinks->GetCells(13);
  cout << "   Center (ncells, cells): " << ncells << " (";
  for (int i=0; i<ncells; ++i)
  {
    cout << imcells[i];
    if ( i < (ncells-1) ) cout << "," ;
  }
  cout << ")\n";
  if ( ncells != 8 )
  {
    return EXIT_FAILURE;
  }

  ncells = imlinks->GetNumberOfCells(26);
  imcells = imlinks->GetCells(26);
  cout << "   Upper Right corner (ncells, cells): " << ncells << " (";
  for (int i=0; i<ncells; ++i)
  {
    cout << imcells[i];
    if ( i < (ncells-1) ) cout << "," ;
  }
  cout << ")\n";
  if ( ncells != 1 || imcells[0] != 7 )
  {
    return EXIT_FAILURE;
  }

  //----------------------------------------------------------------------------
  // Unstructured grid
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

  // Grab the output, build links on unstructured grid
  vtkSmartPointer<vtkUnstructuredGrid> ugrid = extract->GetOutput();

  vtkStaticCellLinksTemplate<int> slinks;
  slinks.BuildLinks(ugrid);

  int numCells = slinks.GetNumberOfCells(0);
  const int *cells = slinks.GetCells(0);
  cout << "\nUnstructured Grid:\n";
  cout << "   Lower Left corner (numCells, cells): " << numCells << " (";
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
  cout << "   Center (numCells, cells): " << numCells << " (";
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
  cout << "   Upper Right corner (numCells, cells): " << numCells << " (";
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

  //----------------------------------------------------------------------------
  // Polydata
  vtkSmartPointer<vtkSphereSource> ss =
    vtkSmartPointer<vtkSphereSource>::New();
  ss->SetThetaResolution(12);
  ss->SetPhiResolution(10);
  ss->Update();

  vtkSmartPointer<vtkPolyData> pdata = ss->GetOutput();

  slinks.Initialize(); //reuse
  slinks.BuildLinks(pdata);

  // The first point is at the pole
  numCells = slinks.GetNumberOfCells(0);
  cells = slinks.GetCells(0);
  cout << "\nPolydata:\n";
  cout << "   Pole: (numCells, cells): " << numCells << " (";
  for (int i=0; i<numCells; ++i)
  {
    cout << cells[i];
    if ( i < (numCells-1) ) cout << "," ;
  }
  cout << ")\n";
  if ( numCells != 12 )
  {
    return EXIT_FAILURE;
  }

  // The next point is at near the equator
  numCells = slinks.GetNumberOfCells(5);
  cells = slinks.GetCells(5);
  cout << "   Equator: (numCells, cells): " << numCells << " (";
  for (int i=0; i<numCells; ++i)
  {
    cout << cells[i];
    if ( i < (numCells-1) ) cout << "," ;
  }
  cout << ")\n";
  if ( numCells != 6 )
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
