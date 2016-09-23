/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGeoView.cxx

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

#include "vtkBMPReader.h"
#include "vtkCamera.h"
#include "vtkGeoAlignedImageRepresentation.h"
#include "vtkGeoAlignedImageSource.h"
#include "vtkGeoEdgeStrategy.h"
#include "vtkGeoFileImageSource.h"
#include "vtkGeoFileTerrainSource.h"
#include "vtkGeoGlobeSource.h"
#include "vtkGeoProjection.h"
#include "vtkGeoProjectionSource.h"
#include "vtkGeoRandomGraphSource.h"
#include "vtkGeoSphereTransform.h"
#include "vtkGeoTerrain.h"
#include "vtkGeoTerrainNode.h"
#include "vtkGeoTerrain2D.h"
#include "vtkGeoTransform.h"
#include "vtkGeoView.h"
#include "vtkGeoView2D.h"
#include "vtkGraphLayoutView.h"
#include "vtkJPEGReader.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderedGraphRepresentation.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTestUtilities.h"
#include "vtkTIFFReader.h"
#include "vtkViewTheme.h"
#include "vtkViewUpdater.h"

#include <vtksys/SystemTools.hxx>

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New();

int TestGeoView(int argc, char* argv[])
{
  char* image2 = vtkTestUtilities::ExpandDataFileName(
      argc, argv, "Data/masonry-wide.jpg");
  char* image = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/NE2_ps_bath_small.jpg");
  vtkStdString imageReadPath = ".";
  vtkStdString imageSavePath = ".";
  vtkStdString imageFile = image;
  vtkStdString terrainReadPath = ".";
  vtkStdString terrainSavePath = ".";
  for (int i = 1; i < argc; ++i)
  {
    if (!strcmp(argv[i], "-I"))
    {
      continue;
    }
    if (!strcmp(argv[i], "-D") ||
        !strcmp(argv[i], "-T") ||
        !strcmp(argv[i], "-V"))
    {
      ++i;
      continue;
    }
    if (!strcmp(argv[i], "-IS"))
    {
      imageSavePath = argv[++i];
    }
    else if (!strcmp(argv[i], "-TS"))
    {
      terrainSavePath = argv[++i];
    }
    else if (!strcmp(argv[i], "-IF"))
    {
      imageFile = argv[++i];
    }
    else if (!strcmp(argv[i], "-IR"))
    {
      imageReadPath = argv[++i];
    }
    else if (!strcmp(argv[i], "-TR"))
    {
      terrainReadPath = argv[++i];
    }
    else
    {
      cerr << "Usage:" << endl;
      cerr << "  -I       - Interactive." << endl;
      cerr << "  -D  path - Path to VTKData." << endl;
      cerr << "  -T  path - Image comparison path." << endl;
      cerr << "  -V  file - Image comparison file." << endl;
      cerr << "  -IS path - Path to save image database to." << endl;
      cerr << "  -TS path - Path to save terrain database to." << endl;
      cerr << "  -IR path - Path to read image database from." << endl;
      cerr << "  -TR path - Path to read terrain database from." << endl;
      cerr << "  -IF file - Load JPEG image file." << endl;
      return 1;
    }
  }
  // Create the geo view.
  VTK_CREATE(vtkGeoView, view);
  view->DisplayHoverTextOff();
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->SetSize(400,400);

  vtkSmartPointer<vtkGeoTerrain> terrain =
    vtkSmartPointer<vtkGeoTerrain>::New();
  vtkSmartPointer<vtkGeoSource> terrainSource;
  vtkGeoGlobeSource* globeSource = vtkGeoGlobeSource::New();
  terrainSource.TakeReference(globeSource);
  terrainSource->Initialize();
  terrain->SetSource(terrainSource);
  view->SetTerrain(terrain);

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

  // Add second image representation
  VTK_CREATE(vtkJPEGReader, reader2);
  reader2->SetFileName(image2);
  reader2->Update();
  vtkSmartPointer<vtkGeoAlignedImageSource> imageSource2 =
    vtkSmartPointer<vtkGeoAlignedImageSource>::New();
  imageSource2->SetImage(reader2->GetOutput());
  vtkSmartPointer<vtkGeoAlignedImageRepresentation> imageRep2 =
    vtkSmartPointer<vtkGeoAlignedImageRepresentation>::New();
  imageSource2->Initialize();
  imageRep2->SetSource(imageSource2);
  view->AddRepresentation(imageRep2);

  // Serialize databases
  if (terrainSavePath.length() > 0)
  {
    terrain->SaveDatabase(terrainSavePath.c_str(), 4);
  }
  if (imageSavePath.length() > 0)
  {
    imageRep->SaveDatabase(imageSavePath.c_str());
  }

  // Load databases
  if (terrainReadPath.length() > 0)
  {
    terrainSource->ShutDown();
    vtkGeoFileTerrainSource* source = vtkGeoFileTerrainSource::New();
    source->SetPath(terrainReadPath.c_str());
    terrainSource.TakeReference(source);
    terrainSource->Initialize();
  }
  terrain->SetSource(terrainSource);
  if (imageReadPath.length() > 0)
  {
    imageSource->ShutDown();
    vtkGeoFileImageSource* source = vtkGeoFileImageSource::New();
    source->SetPath(imageReadPath.c_str());
    imageSource.TakeReference(source);
    imageSource->Initialize();
  }
  imageRep->SetSource(imageSource);

  view->ResetCamera();
  view->GetRenderer()->GetActiveCamera()->Zoom(1.2);

  // Add a graph representation
  vtkSmartPointer<vtkGeoRandomGraphSource> graphSource =
    vtkSmartPointer<vtkGeoRandomGraphSource>::New();
  graphSource->SetNumberOfVertices(100);
  graphSource->StartWithTreeOn();
  graphSource->SetNumberOfEdges(0);
  vtkSmartPointer<vtkRenderedGraphRepresentation> graphRep =
    vtkSmartPointer<vtkRenderedGraphRepresentation>::New();
  graphRep->SetInputConnection(graphSource->GetOutputPort());
  graphRep->SetLayoutStrategyToAssignCoordinates("longitude", "latitude");
  VTK_CREATE(vtkGeoEdgeStrategy, edgeStrategy);
  graphRep->SetEdgeLayoutStrategy(edgeStrategy);
  view->AddRepresentation(graphRep);

  vtkViewTheme* theme = vtkViewTheme::New();
  view->ApplyViewTheme(theme);
  theme->Delete();

  view->Render();

  // BUG: Need to call it twice in order to show the imagery on the globe.
  view->Render();

  // Delay it for 2 secs.
  vtksys::SystemTools::Delay(2000);
  int retVal = vtkRegressionTestImageThreshold(view->GetRenderWindow(), 11);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    // Interact with data.
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();

    retVal = vtkRegressionTester::PASSED;
  }

  // Shut down sources
  terrainSource->ShutDown();
  imageSource->ShutDown();
  imageSource2->ShutDown();

  delete [] image;
  delete [] image2;
  return !retVal;
}

