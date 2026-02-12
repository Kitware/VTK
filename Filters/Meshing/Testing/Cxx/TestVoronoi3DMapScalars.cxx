// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// This test creates a single sphere inside a volume and then uses scalar mapping
// to transform some of the distance values into labeled regions. It then extracts
// a surface net.

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkDataSetMapper.h"
#include "vtkDoubleArray.h"
#include "vtkFeatureEdges.h"
#include "vtkGeneralizedSurfaceNets3D.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkJogglePoints.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataWriter.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSMPTools.h"
#include "vtkTesting.h"
#include "vtkThresholdScalars.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoronoiFlower3D.h"
#include "vtkVoronoiHull.h"

#include <iostream>

namespace
{ // anonymous
// Sample sphere of radius R centered at (0,0,0).
void LabelVoxels(vtkImageData* volume, vtkPoints* points, vtkDataArray* scalars)
{
  int dims[3];
  volume->GetDimensions(dims);
  vtkIdType numPts = volume->GetNumberOfPoints();
  assert(numPts == (dims[0] * dims[1] * dims[2]));

  double* pts = vtkDoubleArray::FastDownCast(points->GetData())->GetPointer(0);
  double* sPtr = vtkDoubleArray::FastDownCast(scalars)->GetPointer(0);

  double origin[3] = { 0, 0, 0 };

  // Make the sphere sampling a little faster
  vtkSMPTools::For(0, numPts,
    [&volume, &pts, &sPtr, &origin](vtkIdType ptId, vtkIdType endPtId)
    {
      for (; ptId < endPtId; ++ptId)
      {
        double x[3];
        volume->GetPoint(ptId, x);
        pts[3 * ptId] = x[0];
        pts[3 * ptId + 1] = x[1];
        pts[3 * ptId + 2] = x[2];
        sPtr[ptId] = sqrt(vtkMath::Distance2BetweenPoints(x, origin));
      }
    }); // end lambda
} // LabelVoxels()
} // anonymous

namespace VoronoiHullDebug
{
// This function is used to debug Voronoi hull clipping -- call it inside
// vtkVoronoiHull to produce an output vtkPolyData file. This file can be
// viewed to understand what is happening during construction of a Voronoi
// hull.
void WriteClipData(vtkVoronoiHull* hull, double origin[3], double normal[3], char* filename)
{
  // Produce the hull as polydata
  vtkNew<vtkPolyData> pd;
  hull->ProducePolyData(pd);

  // Produce the current clipping plane
  double r = sqrt(hull->GetCircumFlower2());
  vtkNew<vtkPlaneSource> ps;
  ps->SetOrigin(0, 0, 0);
  ps->SetPoint1(r, 0, 0);
  ps->SetPoint2(0, r, 0);
  ps->SetCenter(origin);
  ps->SetNormal(normal);
  ps->Update();

  // Append the clipping plane to the data
  vtkNew<vtkAppendPolyData> append;
  append->AddInputData(pd);
  append->AddInputData(ps->GetOutput());

  // Now write it out
  vtkNew<vtkPolyDataWriter> w;
  w->SetFileName(filename);
  w->SetInputConnection(append->GetOutputPort());
  w->Write();
}

bool IsValid(vtkVoronoiHull* hull, char* filename)
{
  vtkNew<vtkPolyData> pd;
  hull->ProducePolyData(pd);

  vtkNew<vtkFeatureEdges> feats;
  feats->SetInputData(pd);
  feats->ExtractAllEdgeTypesOff();
  feats->BoundaryEdgesOn();
  feats->Update();

  if (feats->GetOutput()->GetNumberOfCells() > 0)
  {
    if (filename)
    {
      vtkNew<vtkPolyDataWriter> w;
      w->SetFileName(filename);
      w->SetInputConnection(feats->GetOutputPort());
      w->Write();
    }
    return false;
  }
  return true;
}

} // VoronoiHullDebug namespace

int TestVoronoi3DMapScalars(int argc, char* argv[])
{
  std::cout << "Starting\n";

  // Create a volume. This will be used to generate points and
  // associated scalars.
  //  int DIM = 51;
  int DIM = 9;
  vtkNew<vtkImageData> volume;
  volume->SetDimensions(DIM, DIM, DIM);
  volume->SetOrigin(-DIM / 2, -DIM / 2, -DIM / 2);
  volume->SetSpacing(1, 1, 1);
  volume->AllocateScalars(VTK_DOUBLE, 1);
  vtkDataArray* scalars = volume->GetPointData()->GetScalars();
  vtkIdType numPts = scalars->GetNumberOfTuples();

  // Label the sphere region with distance values
  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToDouble();
  pts->SetNumberOfPoints(numPts);
  LabelVoxels(volume, pts, scalars);

  // Create an instance of vtkPointSet
  vtkNew<vtkUnstructuredGrid> ugrid;
  ugrid->SetPoints(pts);
  ugrid->GetPointData()->SetScalars(scalars);

  // Joggle the points. This removes numerical degeneracies.
  vtkNew<vtkJogglePoints> joggle;
  joggle->SetInputData(ugrid);
  joggle->SetRadius(0.000001);
  joggle->RadiusIsAbsoluteOn();
  joggle->SetJoggleToUnconstrained();
  joggle->Update();

  // Segment the point cloud
  vtkNew<vtkThresholdScalars> labeler;
  labeler->SetInputData(ugrid); // Use this input to test for degeneracies
  //  labeler->SetInputConnection(joggle->GetOutputPort());
  labeler->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ImageScalars");
  labeler->SetBackgroundLabel(-100);
  labeler->AddInterval(5, 10, 1);
  labeler->AddInterval(15, 20, 2);
  labeler->Update();

  // Surface net with mapped scalar data
  vtkNew<vtkGeneralizedSurfaceNets3D> snet;
  snet->SetInputConnection(labeler->GetOutputPort());
  snet->SetNumberOfLabels(2);
  snet->SetLabel(0, 1);
  snet->SetLabel(1, 2);
  snet->BoundaryCappingOn();
  snet->MergePointsOn();
  snet->SmoothingOn();
  snet->SetNumberOfIterations(50);
  snet->SetConstraintDistance(1);
  snet->GenerateSmoothingStencilsOn();
  snet->ValidateOn();
  snet->SetPruneTolerance(1e-13);
  snet->Update();

  vtkNew<vtkVoronoiFlower3D> v;
  v->SetInputConnection(labeler->GetOutputPort());
  v->SetGenerateCellScalarsToPointIds();
  v->Update();

  vtkNew<vtkDataSetMapper> vMapper;
  vMapper->SetInputConnection(v->GetOutputPort());
  // vMapper->ScalarVisibilityOff();
  vMapper->SetScalarRange(0, v->GetOutput()->GetNumberOfPoints());
  // vMapper->SetScalarModeToUseCellData();
  // vMapper->SetLookupTable(lut);

  vtkNew<vtkActor> vActor;
  vActor->SetMapper(vMapper);
  vActor->GetProperty()->SetColor(1, 1, 1);

  vtkNew<vtkLookupTable> lut;
  lut->SetNumberOfTableValues(3);
  lut->Build();
  lut->SetTableValue(0, 0, 0, 0, 1);
  lut->SetTableValue(1, 1, 0, 0, 0.5);
  lut->SetTableValue(2, 0, 1, 0, 0.5);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(snet->GetOutputPort());
  //  mapper->SetScalarRange(0, 2);
  // mapper->SetScalarModeToUseCellData();
  // mapper->SetLookupTable(lut);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1, 1, 1);
  actor->GetProperty()->SetPointSize(2);

  // Report some stats
  std::cout << "Number of input points: " << ugrid->GetNumberOfPoints() << "\n";
  std::cout << "Number of output points: " << snet->GetOutput()->GetNumberOfPoints() << "\n";
  std::cout << "Number of output cells: " << snet->GetOutput()->GetNumberOfCells() << "\n";
  std::cout << "Number of prunes: " << snet->GetNumberOfPrunes() << "\n";

  // Feature edges
  vtkNew<vtkFeatureEdges> feats;
  feats->SetInputConnection(v->GetOutputPort());
  feats->ExtractAllEdgeTypesOff();
  feats->BoundaryEdgesOn();
  feats->Update();

  std::cout << "Number of boundary edges: " << feats->GetOutput()->GetNumberOfCells() << "\n";

  vtkNew<vtkPolyDataMapper> featsMapper;
  featsMapper->SetInputConnection(feats->GetOutputPort());
  featsMapper->ScalarVisibilityOff();

  vtkNew<vtkActor> featsActor;
  featsActor->SetMapper(featsMapper);
  featsActor->GetProperty()->SetColor(1, 0, 0);

  // Bounding box
  vtkNew<vtkOutlineFilter> outline;
  outline->SetInputData(volume);

  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper);

  // Render it all
  vtkNew<vtkRenderer> ren;
  //  ren->AddActor(actor);
  ren->AddActor(vActor);
  ren->AddActor(featsActor);
  ren->AddActor(outlineActor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  iren->Initialize();
  renWin->Render();
  ren->ResetCamera();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
