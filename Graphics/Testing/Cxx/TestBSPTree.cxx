/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBSPTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers intersection of a ray with many polygons
// using the vtkModifiedBSPTree class.
// 
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkSmartPointer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkActor.h"
#include "vtkGlyph3D.h"
#include "vtkLineSource.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkDataSetMapper.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkLookupTable.h"
#include "vtkProperty.h"
#include "vtkPointSource.h"
#include "vtkModifiedBSPTree.h"
#include "vtkBoundingBox.h"
#include "vtkMath.h"
//
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionSource.h"
#include "vtkExtractSelection.h"

#ifdef WIN32
  #include <conio.h>
#endif

//#define TESTING_LOOP

int TestBSPTree(int argc, char* argv[])
{
  // Standard rendering classes
  vtkSmartPointer<vtkRenderer>           renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow>         renWin = vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renWin->AddRenderer(renderer);
  iren->SetRenderWindow(renWin);

  vtkIdType maxI = 0;
  int bestSeed = 0;
  for (int s=931; s<=931; s++) {
    renderer->RemoveAllViewProps();
    
    //
    // Create random point cloud
    //
    vtkMath::RandomSeed(s);
    vtkSmartPointer<vtkPointSource> points = vtkSmartPointer<vtkPointSource>::New();
    points->SetRadius(0.05);
    points->SetNumberOfPoints(30);

    //
    // Create small sphere
    //
    vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
    sphere->SetRadius(0.0125);
    sphere->SetCenter(0.0,0.0,0.0);
    sphere->SetThetaResolution(16);
    sphere->SetPhiResolution(16);
       
    //
    // Glyph many small spheres over point cloud
    //
    vtkSmartPointer<vtkGlyph3D> glyph = vtkSmartPointer<vtkGlyph3D>::New();
    glyph->SetInputConnection(0,points->GetOutputPort(0));
    glyph->SetSourceConnection(sphere->GetOutputPort(0));
    glyph->SetScaling(0); 
    glyph->Update();

    double bounds[6];
    glyph->GetOutput()->GetBounds(bounds);
    vtkBoundingBox box(bounds);
    double tol = box.GetDiagonalLength()/1E6;

    //
    // Intersect Ray with BSP tree full of spheres
    //
    vtkSmartPointer<vtkModifiedBSPTree> BSPTree = vtkSmartPointer<vtkModifiedBSPTree>::New();
    BSPTree->SetDataSet(glyph->GetOutput(0));
    BSPTree->SetMaxLevel(12);
    BSPTree->SetNumberOfCellsPerNode(16);
    BSPTree->BuildLocator();

    vtkSmartPointer<vtkPoints>   verts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();
    double p1[3] = {-0.1, -0.1, -0.1} , p2[3] = {0.1, 0.1, 0.1};
    BSPTree->IntersectWithLine(p1, p2, tol, verts, cellIds);

    vtkSmartPointer<vtkPolyData> intersections = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkCellArray>     vertices = vtkSmartPointer<vtkCellArray>::New();
    vtkIdType N = verts->GetNumberOfPoints();
    vtkIdType *cells = vertices->WritePointer(0, N*2);
    for (int i=0; i<N; i++) {
      cells[i*2]   = 1;  
      cells[i*2+1] = i;  
    }
    intersections->SetPoints(verts);
    intersections->SetVerts(vertices);

    cout << "Seed = " << s << " Number of intersections is " << N << endl;

    vtkSmartPointer<vtkSelectionSource> selection = vtkSmartPointer<vtkSelectionSource>::New();
    vtkSmartPointer<vtkExtractSelection>  extract = vtkSmartPointer<vtkExtractSelection>::New();
    selection->SetContentType(vtkSelectionNode::INDICES);
    selection->SetFieldType(vtkSelectionNode::CELL);
    for (int I=0; I<cellIds->GetNumberOfIds(); I++) {
      cout << cellIds->GetId(I) << ",";
      selection->AddID(-1, cellIds->GetId(I));
    }
    cout << endl;
    //
    extract->SetInputConnection(glyph->GetOutputPort());
    extract->SetSelectionConnection(selection->GetOutputPort());
    extract->Update();

    if (N>maxI) {
      maxI = N;
      bestSeed = s;
    }
    cout << "maxI = " << maxI << " At seed " << bestSeed << endl << endl;

  #ifdef WIN32
      if (kbhit()) {
        char c = getch();
        if (c=='A') break; 
      }
  #endif

#ifndef TESTING_LOOP
    //
    // Render cloud of target spheres
    //
    vtkSmartPointer<vtkPolyDataMapper> Smapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    Smapper->SetInputConnection(glyph->GetOutputPort(0));
      
    vtkSmartPointer<vtkProperty> Sproperty = vtkSmartPointer<vtkProperty>::New();
    Sproperty->SetColor(1.0, 1.0, 1.0);
//    Sproperty->SetOpacity(0.25);
    Sproperty->SetAmbient(0.0);
    Sproperty->SetBackfaceCulling(1);
    Sproperty->SetFrontfaceCulling(0);
    Sproperty->SetRepresentationToPoints();
    Sproperty->SetInterpolationToFlat();

    vtkSmartPointer<vtkActor> Sactor = vtkSmartPointer<vtkActor>::New();
    Sactor->SetMapper(Smapper);
    Sactor->SetProperty(Sproperty);
    renderer->AddActor(Sactor);

    //
    // Render Intersection points
    //
    vtkSmartPointer<vtkGlyph3D> Iglyph = vtkSmartPointer<vtkGlyph3D>::New();
    Iglyph->SetInput(0,intersections);
    Iglyph->SetSourceConnection(sphere->GetOutputPort(0));
    Iglyph->SetScaling(1); 
    Iglyph->SetScaleFactor(0.05); 
    Iglyph->Update();

    vtkSmartPointer<vtkPolyDataMapper> Imapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    Imapper->SetInputConnection(Iglyph->GetOutputPort(0));

    vtkSmartPointer<vtkProperty> Iproperty = vtkSmartPointer<vtkProperty>::New();
    Iproperty->SetOpacity(1.0);
    Iproperty->SetColor(0.0, 0.0, 1.0);
    Iproperty->SetBackfaceCulling(1);
    Iproperty->SetFrontfaceCulling(0);

    vtkSmartPointer<vtkActor> Iactor = vtkSmartPointer<vtkActor>::New();
    Iactor->SetMapper(Imapper);
    Iactor->SetProperty(Iproperty);
    renderer->AddActor(Iactor);

    //
    // Render Ray
    //
    vtkSmartPointer<vtkLineSource> ray = vtkSmartPointer<vtkLineSource>::New();
    ray->SetPoint1(p1);
    ray->SetPoint2(p2);

    vtkSmartPointer<vtkPolyDataMapper> Rmapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    Rmapper->SetInputConnection(ray->GetOutputPort(0));

    vtkSmartPointer<vtkActor> Lactor = vtkSmartPointer<vtkActor>::New();
    Lactor->SetMapper(Rmapper);
    renderer->AddActor(Lactor);

    //
    // Render Intersected Cells (extracted using selection)
    //
    vtkSmartPointer<vtkDataSetMapper> Cmapper = vtkSmartPointer<vtkDataSetMapper>::New();
    Cmapper->SetInputConnection(extract->GetOutputPort(0));

    vtkSmartPointer<vtkProperty> Cproperty = vtkSmartPointer<vtkProperty>::New();
    Cproperty->SetColor(0.0, 1.0, 1.0);
    Cproperty->SetBackfaceCulling(0);
    Cproperty->SetFrontfaceCulling(0);
    Cproperty->SetAmbient(1.0);
    Cproperty->SetLineWidth(3.0);
    Cproperty->SetRepresentationToWireframe();
    Cproperty->SetInterpolationToFlat();

    vtkSmartPointer<vtkActor> Cactor = vtkSmartPointer<vtkActor>::New();
    Cactor->SetMapper(Cmapper);
    Cactor->SetProperty(Cproperty);
    renderer->AddActor(Cactor);

    //
    // Standard testing code.
    //
    renWin->SetSize(300,300);
    renWin->SetMultiSamples(0);
    renWin->Render();
    renderer->GetActiveCamera()->SetPosition(0.0, 0.15, 0.0);
    renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.0, 0.0);
    renderer->GetActiveCamera()->SetViewUp(0.0, 0.0, 1.0);
    renderer->SetBackground(0.0,0.0,0.0);
    renWin->Render();
    renderer->ResetCameraClippingRange();
    renWin->Render();
#endif

  }

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
