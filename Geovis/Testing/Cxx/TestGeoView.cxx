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
#include "vtkGeoFileImageSource.h"
#include "vtkGeoFileTerrainSource.h"
#include "vtkGeoGlobeSource.h"
#include "vtkGeoGraphRepresentation.h"
#include "vtkGeoGraphRepresentation2D.h"
#include "vtkGeoProjection.h"
#include "vtkGeoProjectionSource.h"
#include "vtkGeoRandomGraphSource.h"
#include "vtkGeoTerrain.h"
#include "vtkGeoTerrainNode.h"
#include "vtkGeoTerrain2D.h"
#include "vtkGeoTransform.h"
#include "vtkGeoView.h"
#include "vtkGeoView2D.h"
#include "vtkGraphLayoutView.h"
#include "vtkJPEGReader.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTestUtilities.h"
#include "vtkTIFFReader.h"
#include "vtkViewTheme.h"
#include "vtkViewUpdater.h"

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
  VTK_CREATE(vtkRenderWindow, win);
  win->SetMultiSamples(0);
  VTK_CREATE(vtkGeoView, view);
  view->SetupRenderWindow(win);
  win->SetSize(400,400);

  vtkSmartPointer<vtkGeoTerrain> terrain =
    vtkSmartPointer<vtkGeoTerrain>::New();
  vtkSmartPointer<vtkGeoSource> terrainSource;
  vtkGeoGlobeSource* globeSource = vtkGeoGlobeSource::New();
  terrainSource.TakeReference(globeSource);
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
  imageRep2->SetSource(imageSource2);
  view->AddRepresentation(imageRep2);

  // Add a graph representation
  vtkSmartPointer<vtkGeoRandomGraphSource> graphSource =
    vtkSmartPointer<vtkGeoRandomGraphSource>::New();
  graphSource->SetNumberOfVertices(100);
  graphSource->StartWithTreeOn();
  graphSource->SetNumberOfEdges(0);
  vtkSmartPointer<vtkGeoGraphRepresentation> graphRep =
    vtkSmartPointer<vtkGeoGraphRepresentation>::New();
  graphRep->SetInputConnection(graphSource->GetOutputPort());
  view->AddRepresentation(graphRep);

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
    vtkGeoFileTerrainSource* source = vtkGeoFileTerrainSource::New();
    source->SetPath(terrainReadPath.c_str());
    terrainSource.TakeReference(source);
    }
  terrain->SetSource(terrainSource);
  if (imageReadPath.length() > 0)
    {
    vtkGeoFileImageSource* source = vtkGeoFileImageSource::New();
    source->SetPath(imageReadPath.c_str());
    imageSource.TakeReference(source);
    }
  imageRep->SetSource(imageSource);

  view->Update();

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
#if 0
    // If we are interactive, make some other views to link with.

    // Create a 2D geo view.
    int projIndex = 40;
    vtkSmartPointer<vtkRenderWindow> win2D =
      vtkSmartPointer<vtkRenderWindow>::New();
    vtkSmartPointer<vtkGeoView2D> view2D =
      vtkSmartPointer<vtkGeoView2D>::New();
    view2D->SetupRenderWindow(win2D);
    win2D->SetSize(400,400);

    vtkSmartPointer<vtkGeoAlignedImageSource> imageSource2D =
      vtkSmartPointer<vtkGeoAlignedImageSource>::New();
    imageSource2D->SetImage(reader2->GetOutput());
    vtkSmartPointer<vtkGeoAlignedImageRepresentation> rep2D =
      vtkSmartPointer<vtkGeoAlignedImageRepresentation>::New();
    rep2D->SetSource(imageSource2D);
    view2D->AddRepresentation(rep2D);

    vtkSmartPointer<vtkGeoProjectionSource> grat =
      vtkSmartPointer<vtkGeoProjectionSource>::New();
    grat->SetProjection(projIndex);

    // Set up the viewport
    vtkSmartPointer<vtkGeoTerrainNode> root = vtkSmartPointer<vtkGeoTerrainNode>::New();
    grat->FetchRoot(root);
    double bounds[4];
    root->GetProjectionBounds(bounds);
    view2D->GetRenderer()->GetActiveCamera()->SetParallelScale((bounds[3] - bounds[2]) / 2.0);

    vtkSmartPointer<vtkGeoTerrain2D> surf = vtkSmartPointer<vtkGeoTerrain2D>::New();
    surf->SetSource(grat);
    view2D->SetSurface(surf);

    vtkSmartPointer<vtkGeoGraphRepresentation2D> graphRep2D =
      vtkSmartPointer<vtkGeoGraphRepresentation2D>::New();
    graphRep2D->SetInputConnection(source->GetOutputPort());
    vtkSmartPointer<vtkGeoProjection> projection =
      vtkSmartPointer<vtkGeoProjection>::New();
    projection->SetName(projection->GetProjectionName(projIndex));
    vtkSmartPointer<vtkGeoTransform> transform =
      vtkSmartPointer<vtkGeoTransform>::New();
    transform->SetDestinationProjection(projection);
    graphRep2D->SetTransform(transform);
    view2D->AddRepresentation(graphRep2D);

    // Set up a graph layout view
    vtkSmartPointer<vtkGraphLayoutView> graphView =
      vtkSmartPointer<vtkGraphLayoutView>::New();
    vtkSmartPointer<vtkDataRepresentation> graphRep2 =
      vtkSmartPointer<vtkDataRepresentation>::New();
    graphRep2->SetInputConnection(source->GetOutputPort());
    graphView->AddRepresentation(graphRep2);
    vtkSmartPointer<vtkRenderWindow> graphWin =
      vtkSmartPointer<vtkRenderWindow>::New();
    graphWin->SetSize(400, 400);
    graphView->SetupRenderWindow(graphWin);

    graphRep2->SetSelectionLink(graphRep->GetSelectionLink());
    graphRep2D->SetSelectionLink(graphRep->GetSelectionLink());
    vtkSmartPointer<vtkViewUpdater> updater =
      vtkSmartPointer<vtkViewUpdater>::New();
    updater->AddView(view);
    updater->AddView(view2D);
    updater->AddView(graphView);

    graphView->GetRenderer()->ResetCamera();
    graphView->Update();

    // Make all the themes the same.
    vtkSmartPointer<vtkViewTheme> theme;
    theme.TakeReference(vtkViewTheme::CreateMellowTheme());
    theme->SetSelectedCellColor(1.0, 0.0, 1.0);
    theme->SetSelectedPointColor(1.0, 0.0, 1.0);
    graphView->ApplyViewTheme(theme);
    view->ApplyViewTheme(theme);
    graphRep->ApplyViewTheme(theme);
    graphRep2D->ApplyViewTheme(theme);
#endif

    // Interact with data.
    win->GetInteractor()->Initialize();
    win->GetInteractor()->Start();

    retVal = vtkRegressionTester::PASSED;
    }

  delete [] image;
  delete [] image2;
  return !retVal;
}

