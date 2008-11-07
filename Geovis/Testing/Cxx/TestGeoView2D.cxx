/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGeoView2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .SECTION Description

#include "vtkCamera.h"
#include "vtkGeoAlignedImageSource.h"
#include "vtkGeoAlignedImageRepresentation.h"
#include "vtkGeoProjectionSource.h"
#include "vtkGeoTerrain2D.h"
#include "vtkGeoTerrainNode.h"
#include "vtkGeoView2D.h"
#include "vtkJPEGReader.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTestUtilities.h"
#include "vtkTIFFReader.h"

#include <vtksys/ios/sstream>

int TestGeoView2D(int argc, char* argv[])
{
  int proj = 40;
  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/NE2_ps_bath_small.jpg");
  //char* fname = vtkTestUtilities::ExpandDataFileName(
  //  argc, argv, "Data/world.topo.bathy.200407.3x21600x10800.jpg");
  //char* fname = vtkTestUtilities::ExpandDataFileName(
  //  argc, argv, "Data/world.topo.bathy.200407.3x10000x5000.jpg");
  vtkStdString filename = fname;
  double locationTol = 5.0;
  double textureTol = 1.0;
  for (int a = 1; a < argc; a++)
    {
    if (!strcmp(argv[a], "-p"))
      {
      ++a;
      proj = atoi(argv[a]);
      continue;
      }
    if (!strcmp(argv[a], "-f"))
      {
      ++a;
      filename = argv[a];
      continue;
      }
    if (!strcmp(argv[a], "-lt"))
      {
      ++a;
      locationTol = atof(argv[a]);
      continue;
      }
    if (!strcmp(argv[a], "-tt"))
      {
      ++a;
      textureTol = atof(argv[a]);
      continue;
      }
    if (!strcmp(argv[a], "-I"))
      {
      continue;
      }
    if (!strcmp(argv[a], "-D") ||
        !strcmp(argv[a], "-T") ||
        !strcmp(argv[a], "-V"))
      {
      ++a;
      continue;
      }
    cerr
      << "\nUsage:\n"
      << "  -p proj    - Set projection ID proj (default 5)\n"
      << "  -f path - Set the hi-res image file path\n"
      << "  -lt tol  - Set geometry tolerance in pixels (default 5.0)\n"
      << "  -tt tol  - Set texture tolerance in pixels (default 1.0)\n";
    return 0;
    }

  // Create the view
  vtkSmartPointer<vtkRenderWindow> win = vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkGeoView2D> view = vtkSmartPointer<vtkGeoView2D>::New();
  view->SetupRenderWindow(win);

  // Create the sources
  vtkSmartPointer<vtkGeoAlignedImageSource> imageSource =
    vtkSmartPointer<vtkGeoAlignedImageSource>::New();
  vtkSmartPointer<vtkJPEGReader> reader = vtkSmartPointer<vtkJPEGReader>::New();
  reader->SetFileName(filename.c_str());
  reader->Update();
  imageSource->SetImage(reader->GetOutput());
  vtkSmartPointer<vtkGeoAlignedImageRepresentation> rep =
    vtkSmartPointer<vtkGeoAlignedImageRepresentation>::New();
  rep->SetSource(imageSource);
  view->AddRepresentation(rep);

  // Add image representation
  //char* fname2 = vtkTestUtilities::ExpandDataFileName(
  //  argc, argv, "Data/NE2_ps_bath_small.jpg");
  //char* fname2 = vtkTestUtilities::ExpandDataFileName(
  //    argc, argv, "Data/world.topo.bathy.200407.3x21600x10800.jpg");
  char* fname2 = vtkTestUtilities::ExpandDataFileName(
      argc, argv, "Data/masonry-wide.jpg");
  vtkSmartPointer<vtkJPEGReader> reader2 =
    vtkSmartPointer<vtkJPEGReader>::New();
  reader2->SetFileName(fname2);
  reader2->Update();
  vtkSmartPointer<vtkGeoAlignedImageSource> imageSource2 =
    vtkSmartPointer<vtkGeoAlignedImageSource>::New();
  imageSource2->SetImage(reader2->GetOutput());
  vtkSmartPointer<vtkGeoAlignedImageRepresentation> rep2 =
    vtkSmartPointer<vtkGeoAlignedImageRepresentation>::New();
  rep2->SetSource(imageSource2);
  view->AddRepresentation(rep2);

  win->SetSize(600, 600);
  vtkSmartPointer<vtkGeoProjectionSource> grat
    = vtkSmartPointer<vtkGeoProjectionSource>::New();
  grat->SetProjection(proj);
  vtkGeoSource* gratSource = grat;

  // Set up the viewport
  vtkSmartPointer<vtkGeoTerrainNode> root = vtkSmartPointer<vtkGeoTerrainNode>::New();
  gratSource->FetchRoot(root);
  double bounds[4];
  root->GetProjectionBounds(bounds);
  view->GetRenderer()->GetActiveCamera()->SetParallelScale((bounds[3] - bounds[2]) / 2.0);

  vtkSmartPointer<vtkGeoTerrain2D> surf = vtkSmartPointer<vtkGeoTerrain2D>::New();
  surf->SetSource(gratSource);
  view->SetSurface(surf);

  view->Update();
  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    win->GetInteractor()->Initialize();
    win->GetInteractor()->Start();
    }

  grat->ShutDown();
  imageSource->ShutDown();

  delete [] fname;
  delete [] fname2;
  return !retVal;
}

