// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkHexahedron.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkTetra.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVortexCore.h"

#include "vtkXMLPolyDataWriter.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include <array>

namespace
{
enum GridType
{
  Tetrahedra,
  Hexahedra
};

vtkSmartPointer<vtkUnstructuredGrid> constructGrid(int nX, int nY, int nZ, GridType gridType)
{
  vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();

  vtkSmartPointer<vtkPoints> pointArray = vtkSmartPointer<vtkPoints>::New();

  vtkSmartPointer<vtkPointLocator> pointLocator = vtkSmartPointer<vtkPointLocator>::New();
  double bounds[6] = { -1., 1., -1., 1., -1., 1. };
  pointLocator->InitPointInsertion(pointArray, bounds);

  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();

  double p[8][3];
  double dx = (bounds[1] - bounds[0]) / nX;
  double dy = (bounds[3] - bounds[2]) / nY;
  double dz = (bounds[5] - bounds[4]) / nZ;
  for (vtkIdType i = 0; i < 8; i++)
  {
    for (vtkIdType j = 0; j < 3; j++)
    {
      p[i][j] = bounds[2 * j];
    }
  }
  p[1][0] += dx;
  p[2][0] += dx;
  p[2][1] += dy;
  p[3][1] += dy;
  p[5][0] += dx;
  p[5][2] += dz;
  p[6][0] += dx;
  p[6][1] += dy;
  p[6][2] += dz;
  p[7][1] += dy;
  p[7][2] += dz;

  auto addTetra = [](const double* p0, const double* p1, const double* p2, const double* p3,
                    vtkPointLocator* pl, vtkCellArray* cells) {
    vtkSmartPointer<vtkTetra> t = vtkSmartPointer<vtkTetra>::New();
    static vtkIdType bIndices[4][4] = { { 0, 0, 0, 1 }, { 1, 0, 0, 0 }, { 0, 1, 0, 0 },
      { 0, 0, 1, 0 } };

    vtkIdType nPoints = 4;

    t->GetPointIds()->SetNumberOfIds(nPoints);
    t->GetPoints()->SetNumberOfPoints(nPoints);
    t->Initialize();
    double p_[3];
    vtkIdType pId;
    vtkIdType* bIndex;
    for (vtkIdType i = 0; i < nPoints; i++)
    {
      bIndex = bIndices[i];
      for (vtkIdType j = 0; j < 3; j++)
      {
        p_[j] =
          (p0[j] * bIndex[3]) + (p1[j] * bIndex[0]) + (p2[j] * bIndex[1]) + (p3[j] * bIndex[2]);
      }
      pl->InsertUniquePoint(p_, pId);
      t->GetPointIds()->SetId(i, pId);
      if (i == 3)
        break;
    }
    cells->InsertNextCell(t);
  };

  auto addHex = [](double pts[8][3], vtkPointLocator* pl, vtkCellArray* cells) {
    vtkSmartPointer<vtkHexahedron> h = vtkSmartPointer<vtkHexahedron>::New();

    vtkIdType nPoints = 8;

    h->GetPointIds()->SetNumberOfIds(nPoints);
    h->GetPoints()->SetNumberOfPoints(nPoints);
    h->Initialize();
    vtkIdType pId;
    for (vtkIdType i = 0; i < nPoints; i++)
    {
      pl->InsertUniquePoint(pts[i], pId);
      h->GetPointIds()->SetId(i, pId);
    }
    cells->InsertNextCell(h);
  };

  for (vtkIdType xInc = 0; xInc < nX; xInc++)
  {
    p[0][1] = p[1][1] = p[4][1] = p[5][1] = bounds[2];
    p[2][1] = p[3][1] = p[6][1] = p[7][1] = bounds[2] + dy;

    for (vtkIdType yInc = 0; yInc < nY; yInc++)
    {
      p[0][2] = p[1][2] = p[2][2] = p[3][2] = bounds[4];
      p[4][2] = p[5][2] = p[6][2] = p[7][2] = bounds[4] + dz;

      for (vtkIdType zInc = 0; zInc < nZ; zInc++)
      {
        if (gridType == Tetrahedra)
        {
          if ((xInc + yInc + zInc) % 2 == 0)
          {
            addTetra(p[0], p[1], p[2], p[5], pointLocator, cellArray);
            addTetra(p[0], p[2], p[3], p[7], pointLocator, cellArray);
            addTetra(p[0], p[5], p[7], p[4], pointLocator, cellArray);
            addTetra(p[2], p[5], p[6], p[7], pointLocator, cellArray);
            addTetra(p[0], p[2], p[5], p[7], pointLocator, cellArray);
          }
          else
          {
            addTetra(p[1], p[2], p[3], p[6], pointLocator, cellArray);
            addTetra(p[1], p[3], p[0], p[4], pointLocator, cellArray);
            addTetra(p[1], p[6], p[4], p[5], pointLocator, cellArray);
            addTetra(p[3], p[6], p[7], p[4], pointLocator, cellArray);
            addTetra(p[1], p[3], p[6], p[4], pointLocator, cellArray);
          }
        }
        if (gridType == Hexahedra)
        {
          addHex(p, pointLocator, cellArray);
        }

        for (vtkIdType i = 0; i < 8; i++)
        {
          p[i][2] += dz;
        }
      }

      for (vtkIdType i = 0; i < 8; i++)
      {
        p[i][1] += dy;
      }
    }

    for (vtkIdType i = 0; i < 8; i++)
    {
      p[i][0] += dx;
    }
  }

  unstructuredGrid->SetPoints(pointArray);
  if (gridType == Tetrahedra)
  {
    unstructuredGrid->SetCells(VTK_TETRA, cellArray);
  }
  if (gridType == Hexahedra)
  {
    unstructuredGrid->SetCells(VTK_HEXAHEDRON, cellArray);
  }

  return unstructuredGrid;
}

void constructVelocityProfile(vtkUnstructuredGrid* unstructuredGrid)
{
  auto velocity = [](const double p[3]) -> std::array<double, 3> {
    const double s = .5;
    const double r = .5;
    const double k = .1;

    const double& x = p[0];
    const double& y = p[1];
    const double& z = p[2];

    return { -s * y + r * s * sin(k * z), s * x - r * s * cos(k * z), 1. };
  };

  vtkPoints* pointArray = unstructuredGrid->GetPoints();

  vtkNew<vtkDoubleArray> velocityArray;
  velocityArray->SetName("velocity");
  velocityArray->SetNumberOfComponents(3);
  velocityArray->SetNumberOfTuples(pointArray->GetNumberOfPoints());

  for (int i = 0; i < pointArray->GetNumberOfPoints(); ++i)
  {
    velocityArray->SetTuple(i, velocity(pointArray->GetPoint(i)).data());
  }

  unstructuredGrid->GetPointData()->AddArray(velocityArray);
  unstructuredGrid->GetPointData()->SetActiveVectors("velocity");
}
}

int TestVortexCore(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  vtkSmartPointer<vtkPolyData> output1;
  {
    vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = constructGrid(5, 5, 5, Hexahedra);
    constructVelocityProfile(unstructuredGrid);

    vtkNew<vtkVortexCore> vortexCore;
    vortexCore->FasterApproximationOn();
    vortexCore->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
    vortexCore->SetInputData(unstructuredGrid);
    vortexCore->Update();

    output1 = vtkPolyData::SafeDownCast(vortexCore->GetOutput());
  }

  vtkSmartPointer<vtkPolyData> output2;
  {
    vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = constructGrid(5, 5, 5, Tetrahedra);

    constructVelocityProfile(unstructuredGrid);

    vtkNew<vtkVortexCore> vortexCore;
    vortexCore->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
    vortexCore->SetInputData(unstructuredGrid);
    vortexCore->Update();

    output2 = vtkPolyData::SafeDownCast(vortexCore->GetOutput());
  }

  if (!output1->GetNumberOfCells() || output1->GetNumberOfCells() != output2->GetNumberOfCells())
  {
    std::cerr << "Number of output cells in outputs did not match" << std::endl;
    return EXIT_FAILURE;
  }

  if (output1->CheckAttributes())
  {
    std::cerr << "Output 1 attribute check failed" << std::endl;
    return EXIT_FAILURE;
  }

  if (output2->CheckAttributes())
  {
    std::cerr << "Output 2 attribute check failed" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
