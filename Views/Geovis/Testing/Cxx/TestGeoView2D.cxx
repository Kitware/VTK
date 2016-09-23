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
#include "vtkGeoFileImageSource.h"
#include "vtkGeoFileTerrainSource.h"
#include "vtkGeoProjectionSource.h"
#include "vtkGeoTerrain2D.h"
#include "vtkGeoTerrainNode.h"
#include "vtkGeoView2D.h"
#include "vtkJPEGReader.h"
#include "vtkPolyData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTestUtilities.h"
#include "vtkTIFFReader.h"

#include <sstream>
#include <vtksys/SystemTools.hxx>

int TestGeoView2D(int argc, char* argv[])
{
  int proj = 40;
  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/NE2_ps_bath_small.jpg");
  vtkStdString imageFile = fname;
  vtkStdString imageReadPath = ".";
  vtkStdString imageSavePath = ".";
  vtkStdString terrainReadPath = ".";
  vtkStdString terrainSavePath = ".";
  for (int a = 1; a < argc; a++)
  {
    if (!strcmp(argv[a], "-P"))
    {
      ++a;
      proj = atoi(argv[a]);
      continue;
    }
    if (!strcmp(argv[a], "-IF"))
    {
      ++a;
      imageFile = argv[a];
      continue;
    }
    if (!strcmp(argv[a], "-IR"))
    {
      ++a;
      imageReadPath = argv[a];
      continue;
    }
    if (!strcmp(argv[a], "-IS"))
    {
      ++a;
      imageSavePath = argv[a];
      continue;
    }
    if (!strcmp(argv[a], "-TR"))
    {
      ++a;
      terrainReadPath = argv[a];
      continue;
    }
    if (!strcmp(argv[a], "-TS"))
    {
      ++a;
      terrainSavePath = argv[a];
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
      << "  -P  proj - Projection ID (default 40)\n"
      << "  -IF file - Image file\n"
      << "  -IR path - Image database read path\n"
      << "  -IS path - Image database save path\n"
      << "  -TR file - Terrain databse read path\n"
      << "  -TS file - Terrain databse save path\n"
      << "  -LT tol  - Set geometry tolerance in pixels (default 5.0)\n"
      << "  -TT tol  - Set texture tolerance in pixels (default 1.0)\n";
    return 0;
  }

  // Create the view
  vtkSmartPointer<vtkGeoView2D> view = vtkSmartPointer<vtkGeoView2D>::New();
  view->DisplayHoverTextOff();
  view->GetRenderer()->GradientBackgroundOff();

  // Create the terrain
  vtkSmartPointer<vtkGeoTerrain2D> terrain =
    vtkSmartPointer<vtkGeoTerrain2D>::New();
  vtkSmartPointer<vtkGeoSource> terrainSource;
  vtkGeoProjectionSource* projSource = vtkGeoProjectionSource::New();
  projSource->SetProjection(proj);
  terrainSource.TakeReference(projSource);
  terrainSource->Initialize();
  terrain->SetSource(terrainSource);
  view->SetSurface(terrain);

  // Create background image
  vtkSmartPointer<vtkGeoAlignedImageRepresentation> imageRep =
    vtkSmartPointer<vtkGeoAlignedImageRepresentation>::New();
  vtkSmartPointer<vtkGeoSource> imageSource;
  vtkGeoAlignedImageSource* alignedSource = vtkGeoAlignedImageSource::New();
  vtkSmartPointer<vtkJPEGReader> reader =
    vtkSmartPointer<vtkJPEGReader>::New();
  reader->SetFileName(imageFile.c_str());
  reader->Update();
  alignedSource->SetImage(reader->GetOutput());
  imageSource.TakeReference(alignedSource);
  imageSource->Initialize();
  imageRep->SetSource(imageSource);
  view->AddRepresentation(imageRep);

  // Create second image
  char* fname2 = vtkTestUtilities::ExpandDataFileName(
      argc, argv, "Data/masonry-wide.jpg");
  vtkSmartPointer<vtkJPEGReader> reader2 =
    vtkSmartPointer<vtkJPEGReader>::New();
  reader2->SetFileName(fname2);
  reader2->Update();
  vtkSmartPointer<vtkGeoAlignedImageSource> imageSource2 =
    vtkSmartPointer<vtkGeoAlignedImageSource>::New();
  imageSource2->SetImage(reader2->GetOutput());
  imageSource2->Initialize();
  vtkSmartPointer<vtkGeoAlignedImageRepresentation> imageRep2 =
    vtkSmartPointer<vtkGeoAlignedImageRepresentation>::New();
  imageRep2->SetSource(imageSource2);
  view->AddRepresentation(imageRep2);

  // Serialize databases
  if (imageSavePath.length() > 0)
  {
    imageRep->SaveDatabase(imageSavePath);
  }
  if (terrainSavePath.length() > 0)
  {
    terrain->SaveDatabase(terrainSavePath, 4);
  }

  // Reload databases
  if (terrainReadPath.length() > 0)
  {
    terrainSource->ShutDown();
    vtkGeoFileTerrainSource* source = vtkGeoFileTerrainSource::New();
    source->SetPath(terrainReadPath.c_str());
    source->Initialize();
    terrainSource.TakeReference(source);
  }
  terrain->SetSource(terrainSource);
  if (imageReadPath.length() > 0)
  {
    imageSource->ShutDown();
    vtkGeoFileImageSource* source = vtkGeoFileImageSource::New();
    source->SetPath(imageReadPath.c_str());
    source->Initialize();
    imageSource.TakeReference(source);
  }
  imageRep->SetSource(imageSource);

  // Set up the viewport
  view->GetRenderWindow()->SetSize(600, 600);
  vtkSmartPointer<vtkGeoTerrainNode> root =
    vtkSmartPointer<vtkGeoTerrainNode>::New();
  terrainSource->FetchRoot(root);
  double bounds[6];
  root->GetModel()->GetBounds(bounds);
  bounds[0] = bounds[0] - (bounds[1] - bounds[0])*0.01;
  bounds[1] = bounds[1] + (bounds[1] - bounds[0])*0.01;
  bounds[2] = bounds[2] - (bounds[3] - bounds[2])*0.01;
  bounds[3] = bounds[3] + (bounds[3] - bounds[2])*0.01;
  double scalex = (bounds[1] - bounds[0])/2.0;
  double scaley = (bounds[3] - bounds[2])/2.0;
  double scale = (scalex > scaley) ? scalex : scaley;
  view->GetRenderer()->GetActiveCamera()->SetParallelScale(scale);

  view->Render();
  vtksys::SystemTools::Delay(2000);
  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
  }

  terrainSource->ShutDown();
  imageSource->ShutDown();
  imageSource2->ShutDown();

  delete [] fname;
  delete [] fname2;
  return !retVal;
}

