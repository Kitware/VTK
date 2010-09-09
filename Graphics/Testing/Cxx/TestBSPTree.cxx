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

//#define TESTING_LOOP

int TestBSPTree(int argc, char* argv[])
{
  // Standard rendering classes
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
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
    vtkSmartPointer<vtkModifiedBSPTree> bspTree =
      vtkSmartPointer<vtkModifiedBSPTree>::New();
    bspTree->SetDataSet(glyph->GetOutput(0));
    bspTree->SetMaxLevel(12);
    bspTree->SetNumberOfCellsPerNode(16);
    bspTree->BuildLocator();

    vtkSmartPointer<vtkPoints> verts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();
    double p1[3] = {-0.1, -0.1, -0.1} , p2[3] = {0.1, 0.1, 0.1};
    bspTree->IntersectWithLine(p1, p2, tol, verts, cellIds);

    vtkSmartPointer<vtkPolyData> intersections = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkCellArray>     vertices = vtkSmartPointer<vtkCellArray>::New();
    vtkIdType n = verts->GetNumberOfPoints();
    vtkIdType *cells = vertices->WritePointer(0, n*2);
    for (int i=0; i<n; i++) {
      cells[i*2]   = 1;
      cells[i*2+1] = i;
    }
    intersections->SetPoints(verts);
    intersections->SetVerts(vertices);

    std::cout << "Seed = " << s << " Number of intersections is " << n << std::endl;

    vtkSmartPointer<vtkSelectionSource> selection = vtkSmartPointer<vtkSelectionSource>::New();
    vtkSmartPointer<vtkExtractSelection>  extract = vtkSmartPointer<vtkExtractSelection>::New();
    selection->SetContentType(vtkSelectionNode::INDICES);
    selection->SetFieldType(vtkSelectionNode::CELL);
    for (int i=0; i<cellIds->GetNumberOfIds(); i++) {
      std::cout << cellIds->GetId(i) << ",";
      selection->AddID(-1, cellIds->GetId(i));
    }
    std::cout << std::endl;
    //
    extract->SetInputConnection(glyph->GetOutputPort());
    extract->SetSelectionConnection(selection->GetOutputPort());
    extract->Update();

    if (n>maxI) {
      maxI = n;
      bestSeed = s;
    }
    std::cout << "maxI = " << maxI << " At seed " << bestSeed << std::endl << std::endl;

#ifndef TESTING_LOOP
    //
    // Render cloud of target spheres
    //
    vtkSmartPointer<vtkPolyDataMapper> smapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
    smapper->SetInputConnection(glyph->GetOutputPort(0));

    vtkSmartPointer<vtkProperty> sproperty = vtkSmartPointer<vtkProperty>::New();
    sproperty->SetColor(1.0, 1.0, 1.0);
//  sproperty->SetOpacity(0.25);
    sproperty->SetAmbient(0.0);
    sproperty->SetBackfaceCulling(1);
    sproperty->SetFrontfaceCulling(0);
    sproperty->SetRepresentationToPoints();
    sproperty->SetInterpolationToFlat();

    vtkSmartPointer<vtkActor> sactor =
      vtkSmartPointer<vtkActor>::New();
    sactor->SetMapper(smapper);
    sactor->SetProperty(sproperty);
    renderer->AddActor(sactor);

    //
    // Render Intersection points
    //
    vtkSmartPointer<vtkGlyph3D> iglyph = vtkSmartPointer<vtkGlyph3D>::New();
    iglyph->SetInput(0,intersections);
    iglyph->SetSourceConnection(sphere->GetOutputPort(0));
    iglyph->SetScaling(1);
    iglyph->SetScaleFactor(0.05);
    iglyph->Update();

    vtkSmartPointer<vtkPolyDataMapper> imapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
    imapper->SetInputConnection(iglyph->GetOutputPort(0));

    vtkSmartPointer<vtkProperty> iproperty = vtkSmartPointer<vtkProperty>::New();
    iproperty->SetOpacity(1.0);
    iproperty->SetColor(0.0, 0.0, 1.0);
    iproperty->SetBackfaceCulling(1);
    iproperty->SetFrontfaceCulling(0);

    vtkSmartPointer<vtkActor> iactor = vtkSmartPointer<vtkActor>::New();
    iactor->SetMapper(imapper);
    iactor->SetProperty(iproperty);
    renderer->AddActor(iactor);

    //
    // Render Ray
    //
    vtkSmartPointer<vtkLineSource> ray = vtkSmartPointer<vtkLineSource>::New();
    ray->SetPoint1(p1);
    ray->SetPoint2(p2);

    vtkSmartPointer<vtkPolyDataMapper> rmapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    rmapper->SetInputConnection(ray->GetOutputPort(0));

    vtkSmartPointer<vtkActor> lactor = vtkSmartPointer<vtkActor>::New();
    lactor->SetMapper(rmapper);
    renderer->AddActor(lactor);

    //
    // Render Intersected Cells (extracted using selection)
    //
    vtkSmartPointer<vtkDataSetMapper> cmapper = vtkSmartPointer<vtkDataSetMapper>::New();
    cmapper->SetInputConnection(extract->GetOutputPort(0));

    vtkSmartPointer<vtkProperty> cproperty = vtkSmartPointer<vtkProperty>::New();
    cproperty->SetColor(0.0, 1.0, 1.0);
    cproperty->SetBackfaceCulling(0);
    cproperty->SetFrontfaceCulling(0);
    cproperty->SetAmbient(1.0);
    cproperty->SetLineWidth(3.0);
    cproperty->SetRepresentationToWireframe();
    cproperty->SetInterpolationToFlat();

    vtkSmartPointer<vtkActor> cactor = vtkSmartPointer<vtkActor>::New();
    cactor->SetMapper(cmapper);
    cactor->SetProperty(cproperty);
    renderer->AddActor(cactor);

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
