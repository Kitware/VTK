// SPDX-FileCopyrightText: Copyright (c) Rupert Nash, University of Edinburgh
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// Test case for vtkPolyDataNormals that ensure normals are correctly
// oriented for a slightly contrived tetrahedron.

#include <array>

#include <vtkCellCenters.h>
#include <vtkCellData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataNormals.h>

namespace
{

vtkNew<vtkPolyData> MakeTetrahedron()
{
  vtkNew<vtkPolyData> tet;

  vtkNew<vtkPoints> points;
  points->Allocate(4);
  points->InsertNextPoint(0.00, 0.00, 0.00);
  points->InsertNextPoint(0.10, 0.45, 0.55);
  points->InsertNextPoint(0.10, 0.55, 0.45);
  points->InsertNextPoint(0.05, 0.50, 0.50);

  tet->SetPoints(points);

  int const nTri = 4;
  vtkNew<vtkCellArray> polys;
  polys->AllocateExact(nTri, 3 * nTri);
  polys->InsertNextCell({ 0, 1, 2 });
  polys->InsertNextCell({ 0, 2, 3 });
  polys->InsertNextCell({ 0, 3, 1 });
  polys->InsertNextCell({ 1, 3, 2 });

  tet->SetPolys(polys);
  return tet;
}

std::array<double, 3> ComputeCenter(vtkPolyData* pd)
{
  vtkPoints* p = pd->GetPoints();
  const vtkIdType np = p->GetNumberOfPoints();

  std::array<double, 3> ans{ 0, 0, 0 };
  for (int i = 0; i < np; ++i)
  {
    auto pi = p->GetPoint(i);
    ans[0] += pi[0];
    ans[1] += pi[1];
    ans[2] += pi[2];
  }
  ans[0] /= np;
  ans[1] /= np;
  ans[2] /= np;
  return ans;
}
}

int TestPolyDataNormals(int, char*[])
{
  vtkNew<vtkPolyData> tet = ::MakeTetrahedron();
  vtkIdType nTri = tet->GetNumberOfCells();

  vtkNew<vtkPolyDataNormals> normer;
  normer->ComputeCellNormalsOn();
  normer->ComputePointNormalsOff();
  normer->NonManifoldTraversalOff();
  normer->SplittingOff();

  normer->FlipNormalsOff();
  normer->AutoOrientNormalsOn();
  normer->ConsistencyOn();

  normer->SetInputData(tet);
  normer->Update();

  vtkDataArray* normals = normer->GetOutput()->GetCellData()->GetNormals();

  std::array<double, 3> tetCenter = ::ComputeCenter(tet);

  vtkNew<vtkCellCenters> centerer;
  centerer->SetInputData(tet);
  centerer->Update();
  vtkPoints* triCenters = centerer->GetOutput()->GetPoints();

  for (int i = 0; i < nTri; ++i)
  {
    // Compute the cell center relative to center of tet.
    double dx[3];
    for (int d = 0; d < 3; ++d)
    {
      dx[d] = triCenters->GetPoint(i)[d] - tetCenter[d];
    }
    double const* normal = normals->GetTuple(i);
    // Normal should point in same direction as dx.
    if (vtkMath::Dot(dx, normal) <= 0.0)
    {
      vtkIdType cellSize;
      vtkIdType const* cellPts;
      tet->GetPolys()->GetCellAtId(i, cellSize, cellPts);

      std::cerr << "Inward pointing normal for face ID:" << i << " with points:\n";
      for (int ptIdx = 0; ptIdx < cellSize; ++ptIdx)
      {
        vtkIdType const& ptId = cellPts[ptIdx];
        double r[3];
        tet->GetPoint(ptId, r);
        std::cerr << "ID: " << ptId << "; x: " << r[0] << "; y: " << r[1] << "; z: " << r[2]
                  << "\n";
      }

      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
