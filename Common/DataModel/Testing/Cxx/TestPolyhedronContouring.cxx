// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkClipDataSet.h"
#include "vtkContourFilter.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"

#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkPolyhedron.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <iostream>

// Normalize polyhedron face winding to outward-pointing normals.
// Builds a new grid with corrected face winding for all polyhedra.
static vtkSmartPointer<vtkUnstructuredGrid> NormalizePolyhedronWinding(vtkUnstructuredGrid* input)
{
  vtkNew<vtkUnstructuredGrid> output;
  output->SetPoints(input->GetPoints());
  output->GetPointData()->ShallowCopy(input->GetPointData());
  output->GetCellData()->CopyAllocate(input->GetCellData());
  output->Allocate(input->GetNumberOfCells());

  for (vtkIdType cellId = 0; cellId < input->GetNumberOfCells(); ++cellId)
  {
    if (input->GetCellType(cellId) != VTK_POLYHEDRON)
    {
      // Copy non-polyhedron cells as-is
      vtkNew<vtkIdList> ptIds;
      input->GetCellPoints(cellId, ptIds);
      output->InsertNextCell(input->GetCellType(cellId), ptIds);
      output->GetCellData()->CopyData(input->GetCellData(), cellId, cellId);
      continue;
    }

    vtkCell* cell = input->GetCell(cellId);
    int nFaces = cell->GetNumberOfFaces();

    // Compute cell centroid from points
    double centroid[3] = { 0, 0, 0 };
    for (vtkIdType i = 0; i < cell->GetNumberOfPoints(); ++i)
    {
      double pt[3];
      cell->GetPoints()->GetPoint(i, pt);
      for (int d = 0; d < 3; ++d)
        centroid[d] += pt[d];
    }
    for (int d = 0; d < 3; ++d)
      centroid[d] /= cell->GetNumberOfPoints();

    // Build corrected face stream
    vtkNew<vtkIdList> faceStream;
    faceStream->InsertNextId(nFaces);
    for (int fi = 0; fi < nFaces; ++fi)
    {
      vtkCell* face = cell->GetFace(fi);
      int nFacePts = face->GetNumberOfPoints();
      std::vector<vtkIdType> facePtIds(nFacePts);
      for (int j = 0; j < nFacePts; ++j)
        facePtIds[j] = face->GetPointId(j);

      // Check normal direction
      if (nFacePts >= 3)
      {
        double p0[3], p1[3], p2[3];
        input->GetPoint(facePtIds[0], p0);
        input->GetPoint(facePtIds[1], p1);
        input->GetPoint(facePtIds[2], p2);
        double e1[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
        double e2[3] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };
        double normal[3];
        vtkMath::Cross(e1, e2, normal);

        double faceCtr[3] = { 0, 0, 0 };
        for (int j = 0; j < nFacePts; ++j)
        {
          double pt[3];
          input->GetPoint(facePtIds[j], pt);
          for (int d = 0; d < 3; ++d)
            faceCtr[d] += pt[d];
        }
        for (int d = 0; d < 3; ++d)
          faceCtr[d] /= nFacePts;

        double outward[3] = { faceCtr[0] - centroid[0], faceCtr[1] - centroid[1],
          faceCtr[2] - centroid[2] };

        if (vtkMath::Dot(normal, outward) < 0)
          std::reverse(facePtIds.begin(), facePtIds.end());
      }

      faceStream->InsertNextId(nFacePts);
      for (int j = 0; j < nFacePts; ++j)
        faceStream->InsertNextId(facePtIds[j]);
    }

    // Get point ids
    vtkNew<vtkIdList> ptIds;
    input->GetCellPoints(cellId, ptIds);

    output->InsertNextCell(VTK_POLYHEDRON, ptIds->GetNumberOfIds(), ptIds->GetPointer(0),
      faceStream->GetId(0), faceStream->GetPointer(1));
    output->GetCellData()->CopyData(input->GetCellData(), cellId, cellId);
  }

  return output;
}

int TestPolyhedronContouring(int argc, char* argv[])
{
  vtkObject::GlobalWarningDisplayOff();
  vtkNew<vtkXMLUnstructuredGridReader> r;
  vtkNew<vtkContourFilter> cf;
  cf->GenerateTrianglesOff();

  {
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cell_850113.vtu");
    r->SetFileName(fname);
    r->Update();

    auto grid = NormalizePolyhedronWinding(r->GetOutput());
    cf->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "AirVolumeFraction");
    cf->SetInputData(grid.Get());
    cf->SetValue(0, 0.5);
    cf->Update();

    vtkPolyData* polys = cf->GetOutput();
    if (polys->GetNumberOfCells() != 2)
    {
      std::cerr << "Number of polys not 2 (as expected), but " << polys->GetNumberOfCells()
                << std::endl;
      return EXIT_FAILURE;
    }
    cf->GenerateTrianglesOn();
    cf->Update();
    vtkPolyData* triangles = cf->GetOutput();
    if (triangles->GetNumberOfCells() != 4)
    {
      std::cerr << "Number of triangles is not 4 (as expected), but "
                << triangles->GetNumberOfCells() << std::endl;
      return EXIT_FAILURE;
    }

    vtkNew<vtkClipDataSet> cd;
    cd->SetInputData(grid.Get());
    cd->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "AirVolumeFraction");
    cd->SetValue(0.5);
    cd->SetInsideOut(0);
    cd->Update();

    vtkUnstructuredGrid* clip = cd->GetOutput();
    if (clip->GetNumberOfCells() != 1)
    {
      std::cerr << "Number of 'less' clipped cells not 1 (as expected), but "
                << clip->GetNumberOfCells() << std::endl;
      return EXIT_FAILURE;
    }

    vtkCell* clipCell0 = clip->GetCell(0);
    int nFaces0 = clipCell0->GetNumberOfFaces();
    if (nFaces0 != 10)
    {
      std::cerr << "Expected clipped cell with 10 faces, got " << nFaces0 << " faces." << std::endl;
      return EXIT_FAILURE;
    }

    cd->SetInsideOut(1);
    cd->Update();

    clip = cd->GetOutput();
    if (clip->GetNumberOfCells() != 1)
    {
      std::cerr << "Number of 'greater' clipped cells not 1 (as expected), but "
                << clip->GetNumberOfCells() << std::endl;
      return EXIT_FAILURE;
    }

    vtkCell* clipCell = clip->GetCell(0);
    // Outside clip: 8 side faces + 2 cap polygons. Faces with multiple outside segments are merged.
    if (clipCell->GetNumberOfFaces() != 11)
    {
      std::cerr << "Expected one clipped cell with 11 faces, got " << clipCell->GetNumberOfFaces()
                << " faces." << std::endl;
      return EXIT_FAILURE;
    }
  }

  // yet another problematic case, which gives an incorrect non-watertight warning in the old
  // contouring code
  {
    vtkNew<vtkPoints> pts;
    pts->InsertNextPoint(1, 0, 0);
    pts->InsertNextPoint(.5, 0, 0);
    pts->InsertNextPoint(0, 0, 0);

    pts->InsertNextPoint(1, 1, 1);
    pts->InsertNextPoint(1, 1, .5);
    pts->InsertNextPoint(1, 1, 0);

    pts->InsertNextPoint(1, 0, 1);
    pts->InsertNextPoint(1, .5, 0);
    pts->InsertNextPoint(0, 1, 1);

    pts->InsertNextPoint(.5, 1, 0);
    pts->InsertNextPoint(0, 1, 0);
    pts->InsertNextPoint(0, 0, 1);

    pts->InsertNextPoint(.5, .5, 0);
    pts->InsertNextPoint(0, .5, 0);

    vtkNew<vtkUnstructuredGrid> p;
    p->SetPoints(pts);
    p->Allocate(1);

    vtkIdType faceStream[] = { 6, 8, 3, 4, 5, 9, 10, 4, 11, 6, 3, 8, 6, 3, 6, 0, 7, 5, 4, 4, 9, 5,
      7, 12, 4, 10, 9, 12, 13, 4, 13, 12, 1, 2, 4, 12, 7, 0, 1, 5, 10, 13, 2, 11, 8, 5, 2, 1, 0, 6,
      11 };

    p->InsertNextCell(VTK_POLYHEDRON, 9, faceStream);

    double values[] = { 0.48828, 0.920027, 0.959499, 0.51357, 0.497449, 0.523359, 0.470217,
      0.498483, 0.956751, 0.928612, 0.971497, 0.942868, 0.93052, 0.961309 };

    vtkNew<vtkDoubleArray> arr;
    arr->SetArray(values, 14, 1);
    arr->SetName("AirVolumeFraction");
    p->GetPointData()->AddArray(arr);

    cf->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "AirVolumeFraction");
    cf->SetInputData(p);
    cf->GenerateTrianglesOff();
    cf->SetValue(0, 0.5);
    cf->Update();

    vtkPolyData* result = cf->GetOutput();
    if (result->GetNumberOfCells() != 1)
    {
      std::cerr << "Expected 1 contour polyhedron, got " << result->GetNumberOfCells() << std::endl;
      return EXIT_FAILURE;
    }

    vtkCell* contour = result->GetCell(0);
    if (contour->GetNumberOfPoints() != 7)
    {
      std::cerr << "Expected 7 contour points, got " << contour->GetNumberOfPoints() << std::endl;
      return EXIT_FAILURE;
    }

    cf->GenerateTrianglesOn();
    cf->Update();
    vtkPolyData* triangles = cf->GetOutput();
    if (triangles->GetNumberOfCells() != 5)
    {
      std::cerr << "Expected 5 contour triangles, got " << triangles->GetNumberOfCells()
                << std::endl;
      return EXIT_FAILURE;
    }

    r->SetFileName(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cell_12851_26.vtu"));
    r->Update();

    auto cell_12851 = NormalizePolyhedronWinding(r->GetOutput());
    cf->SetInputData(cell_12851.Get());
    cf->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "AirVolumeFraction");
    cf->SetValue(0, 0.5);
    cf->Update();

    vtkPolyData* cell_12851_contour = cf->GetOutput();

    if (cell_12851_contour->GetNumberOfCells() != 1)
    {
      std::cerr << "cell_12851: Expected 1 contour polyhedron, got "
                << cell_12851_contour->GetNumberOfCells() << std::endl;
      return EXIT_FAILURE;
    }

    contour = cell_12851_contour->GetCell(0);
    if (contour->GetNumberOfPoints() != 3)
    {
      std::cerr << "cell_12851: Expected 3 contour points, got " << contour->GetNumberOfPoints()
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Another problematic case. this one gave "problem in face navigation"
  // in an earlier approach to polyhedron face triangulation.  That problem
  // is now solved in vtkPolyhedron::TriangulatePolyhedralFaces
  {
    vtkNew<vtkPoints> pnts;
    pnts->InsertNextPoint(0.440016, 0.189264, 0.181594);
    pnts->InsertNextPoint(0.440537, 0.188737, 0.180708);
    pnts->InsertNextPoint(0.439976, 0.18893, 0.180698);
    pnts->InsertNextPoint(0.440257, 0.188834, 0.180703);
    pnts->InsertNextPoint(0.440597, 0.18926, 0.181462);
    pnts->InsertNextPoint(0.439896, 0.189791, 0.180785);
    pnts->InsertNextPoint(0.439833, 0.189866, 0.18164);
    pnts->InsertNextPoint(0.440492, 0.189543, 0.180782);
    pnts->InsertNextPoint(0.440567, 0.188999, 0.181085);
    pnts->InsertNextPoint(0.440306, 0.189262, 0.181528);
    pnts->InsertNextPoint(0.440499, 0.189503, 0.181569);
    pnts->InsertNextPoint(0.440166, 0.189685, 0.181605);

    vtkNew<vtkUnstructuredGrid> ba;
    ba->SetPoints(pnts);

    double values[] = { 0.544052, 0.479528, 0.485401, 0.491219, 0.522598, 0.460551, 0.508554,
      0.454234, 0.517886, 0.528239, 0.494647, 0.499257 };

    vtkNew<vtkDoubleArray> data;
    data->SetArray(values, 12, 1);
    data->SetName("AirVolumeFraction");

    ba->GetPointData()->AddArray(data);

    vtkIdType faceStream[] = { 5, 9, 3, 1, 8, 4, 4, 0, 2, 3, 9, 5, 10, 7, 1, 8, 4, 5, 3, 2, 5, 7, 1,

      4, 6, 5, 2, 0, 4, 11, 6, 0, 9, 4, 10, 11, 9, 4, 5, 10, 7, 5, 6, 11 };

    ba->InsertNextCell(VTK_POLYHEDRON, 8, faceStream);

    cf->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "AirVolumeFraction");
    cf->SetInputData(ba);
    cf->SetValue(0, 0.5);
    cf->GenerateTrianglesOff();
    cf->Update();

    vtkPolyData* result = cf->GetOutput();
    if (!result || result->GetNumberOfCells() != 1)
    {
      std::cerr << "Contouring failed for polyhedron cell" << std::endl;
      return EXIT_FAILURE;
    }
    vtkCell* contourCell = result->GetCell(0);

    if (contourCell->GetNumberOfPoints() != 6)
    {
      std::cerr << "Expected contour with 6 points, got " << contourCell->GetNumberOfPoints()
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
