// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPlane.h"
#include "vtkPolyhedron.h"
#include "vtkUnstructuredGrid.h"

#include "vtkCutter.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include "vtkCellData.h"
#include "vtkPointData.h"

#include <algorithm>
#include <iostream>

// Normalize polyhedron face winding to outward-pointing normals.
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
      vtkNew<vtkIdList> ptIds;
      input->GetCellPoints(cellId, ptIds);
      output->InsertNextCell(input->GetCellType(cellId), ptIds);
      output->GetCellData()->CopyData(input->GetCellData(), cellId, cellId);
      continue;
    }

    vtkCell* cell = input->GetCell(cellId);
    int nFaces = cell->GetNumberOfFaces();

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

    vtkNew<vtkIdList> faceStream;
    faceStream->InsertNextId(nFaces);
    for (int fi = 0; fi < nFaces; ++fi)
    {
      vtkCell* face = cell->GetFace(fi);
      int nFacePts = face->GetNumberOfPoints();
      std::vector<vtkIdType> facePtIds(nFacePts);
      for (int j = 0; j < nFacePts; ++j)
        facePtIds[j] = face->GetPointId(j);

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

    vtkNew<vtkIdList> ptIds;
    input->GetCellPoints(cellId, ptIds);
    output->InsertNextCell(VTK_POLYHEDRON, ptIds->GetNumberOfIds(), ptIds->GetPointer(0),
      faceStream->GetId(0), faceStream->GetPointer(1));
    output->GetCellData()->CopyData(input->GetCellData(), cellId, cellId);
  }

  return output;
}

// Test of contour/clip of vtkPolyhedron. uses input from
// https://gitlab.kitware.com/vtk/vtk/-/issues/14485
int TestPolyhedron2(int argc, char* argv[])
{
  vtkObject::GlobalWarningDisplayOff();

  const char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/polyhedron_mesh.vtu");
  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName(filename);
  reader->Update();

  auto pGrid = NormalizePolyhedronWinding(reader->GetOutput());

  vtkNew<vtkCutter> cutter;
  vtkNew<vtkPlane> p;
  p->SetOrigin(pGrid->GetCenter());
  p->SetNormal(1, 0, 0);

  cutter->SetCutFunction(p);
  cutter->SetGenerateTriangles(0);

  cutter->SetInputData(pGrid);
  cutter->Update();

  vtkPolyData* output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  if (output->GetNumberOfCells() != 2)
  {
    std::cerr << "Expected 2 polygons but found " << output->GetNumberOfCells()
              << " polygons in sliced polyhedron." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
