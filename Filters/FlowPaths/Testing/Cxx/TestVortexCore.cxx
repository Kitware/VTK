// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
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

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <vector>

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
                    vtkPointLocator* pl, vtkCellArray* cells)
  {
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

  auto addHex = [](double pts[8][3], vtkPointLocator* pl, vtkCellArray* cells)
  {
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
  auto velocity = [](const double p[3]) -> std::array<double, 3>
  {
    constexpr double s = .5;
    constexpr double r = .5;
    constexpr double k = .1;

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

// Same field stored as float. vtkGradientFilter's output follows its input
// type, so a float velocity yields a float jacobian and jacobian_prime. The
// higher order path used to downcast jacobian_prime to vtkDoubleArray, which
// silently produced null and reached the dispatch worker unchecked.
void constructFloatVelocityProfile(vtkUnstructuredGrid* unstructuredGrid)
{
  vtkPoints* pointArray = unstructuredGrid->GetPoints();

  vtkNew<vtkFloatArray> velocityArray;
  velocityArray->SetName("velocity");
  velocityArray->SetNumberOfComponents(3);
  velocityArray->SetNumberOfTuples(pointArray->GetNumberOfPoints());

  for (int i = 0; i < pointArray->GetNumberOfPoints(); ++i)
  {
    constexpr double s = .5, r = .5, k = .1;
    const double* p = pointArray->GetPoint(i);
    const float v[3] = { static_cast<float>(-s * p[1] + r * s * sin(k * p[2])),
      static_cast<float>(s * p[0] - r * s * cos(k * p[2])), 1.f };
    velocityArray->SetTypedTuple(i, v);
  }

  unstructuredGrid->GetPointData()->AddArray(velocityArray);
  unstructuredGrid->GetPointData()->SetActiveVectors("velocity");
}

// A linear vortex with axial stretching, the core of a Burgers vortex:
// u = (-s y' - a x' / 2, s x' - a y' / 2, a z) about the axis (x0, y0). The
// gradient is exact on any grid, and the vortex core is the axis, placed away
// from grid lines and cell diagonals.
constexpr double StretchedSwirl = 2.;
constexpr double StretchedStrain = 1.;

void constructStretchedVortex(vtkUnstructuredGrid* unstructuredGrid)
{
  constexpr double x0 = 0.137, y0 = -0.071;
  constexpr double s = StretchedSwirl, a = StretchedStrain;
  vtkPoints* pointArray = unstructuredGrid->GetPoints();
  vtkNew<vtkDoubleArray> velocityArray;
  velocityArray->SetName("velocity");
  velocityArray->SetNumberOfComponents(3);
  velocityArray->SetNumberOfTuples(pointArray->GetNumberOfPoints());
  for (vtkIdType i = 0; i < pointArray->GetNumberOfPoints(); ++i)
  {
    const double* p = pointArray->GetPoint(i);
    const double x = p[0] - x0, y = p[1] - y0;
    velocityArray->SetTuple3(i, -s * y - 0.5 * a * x, s * x - 0.5 * a * y, a * p[2]);
  }
  unstructuredGrid->GetPointData()->AddArray(velocityArray);
  unstructuredGrid->GetPointData()->SetActiveVectors("velocity");
}

struct Expected
{
  const char* Name;
  double Value;
};

// For constructVelocityProfile, J = [[0, -s, a], [s, 0, b], [0, 0, 0]] with a
// and b depending only on z and s = 0.5. Its eigenvalues are 0 and +/- i s, the
// a and b terms cancel in Q and lambda_2, and det J = 0:
// Q = s^2, delta = (Q / 3)^3, lambda_ci = s, lambda_2 = -s^2.
std::vector<Expected> SwirlCriteria()
{
  constexpr double s = 0.5;
  const double q = s * s;
  return { { "q-criterion", q }, { "delta-criterion", std::pow(q / 3., 3.) },
    { "lambda_ci-criterion", s }, { "lambda_2-criterion", -s * s } };
}

// For constructStretchedVortex, J = [[-a/2, -s, 0], [s, -a/2, 0], [0, 0, a]]:
// S = diag(-a/2, -a/2, a), so Q = s^2 - 3 a^2 / 4; det J = a (a^2 / 4 + s^2);
// the eigenvalues are a and -a/2 +/- i s; S^2 + Omega^2 has eigenvalues
// a^2 / 4 - s^2 (twice) and a^2. Computing J at half its value or Q from
// spectral norms gives Q = (s - a) / 2, det J / 8 and lambda_ci = s / 2.
std::vector<Expected> StretchedCriteria()
{
  constexpr double s = StretchedSwirl, a = StretchedStrain;
  const double q = s * s - 0.75 * a * a;
  const double determinant = a * (0.25 * a * a + s * s);
  return { { "q-criterion", q },
    { "delta-criterion", std::pow(q / 3., 3.) + 0.25 * determinant * determinant },
    { "lambda_ci-criterion", s }, { "lambda_2-criterion", 0.25 * a * a - s * s } };
}

// Reports every criterion that differs from its closed form, not just the first.
bool CheckCriteria(vtkPolyData* output, const char* label, const std::vector<Expected>& expected)
{
  if (output->GetNumberOfPoints() == 0)
  {
    std::cerr << label << ": no vortex core points" << std::endl;
    return false;
  }
  bool ok = true;
  for (const auto& e : expected)
  {
    vtkDataArray* array = output->GetPointData()->GetArray(e.Name);
    if (!array || array->GetNumberOfTuples() == 0)
    {
      std::cerr << label << ": missing " << e.Name << std::endl;
      ok = false;
      continue;
    }
    for (vtkIdType i = 0; i < array->GetNumberOfTuples(); ++i)
    {
      const double value = array->GetComponent(i, 0);
      if (std::abs(value - e.Value) > 1e-6 * std::max(1., std::abs(e.Value)))
      {
        std::cerr << label << ": " << e.Name << " is " << value << ", expected " << e.Value
                  << std::endl;
        ok = false;
        break;
      }
    }
  }
  return ok;
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

  bool criteriaOk = CheckCriteria(output1, "Hexahedra", SwirlCriteria());
  criteriaOk = CheckCriteria(output2, "Tetrahedra", SwirlCriteria()) && criteriaOk;
  for (GridType gridType : { Hexahedra, Tetrahedra })
  {
    vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = constructGrid(5, 5, 5, gridType);
    constructStretchedVortex(unstructuredGrid);

    vtkNew<vtkVortexCore> vortexCore;
    vortexCore->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
    vortexCore->SetInputData(unstructuredGrid);
    vortexCore->Update();
    criteriaOk =
      CheckCriteria(vortexCore->GetOutput(),
        gridType == Hexahedra ? "Stretched vortex, hexahedra" : "Stretched vortex, tetrahedra",
        StretchedCriteria()) &&
      criteriaOk;
  }
  if (!criteriaOk)
  {
    return EXIT_FAILURE;
  }

  // Higher order method on a float velocity field.
  {
    vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = constructGrid(5, 5, 5, Hexahedra);
    constructFloatVelocityProfile(unstructuredGrid);

    if (!vtkFloatArray::SafeDownCast(unstructuredGrid->GetPointData()->GetArray("velocity")))
    {
      std::cerr << "Velocity must be float for this case" << std::endl;
      return EXIT_FAILURE;
    }

    vtkNew<vtkVortexCore> vortexCore;
    vortexCore->SetHigherOrderMethod(true);
    vortexCore->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
    vortexCore->SetInputData(unstructuredGrid);
    vortexCore->Update();

    vtkPolyData* output3 = vtkPolyData::SafeDownCast(vortexCore->GetOutput());
    if (!output3 || output3->CheckAttributes())
    {
      std::cerr << "Float higher order output attribute check failed" << std::endl;
      return EXIT_FAILURE;
    }
    if (!output3->GetNumberOfCells())
    {
      std::cerr << "Float higher order method produced no cells" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // On the stretched vortex, velocity, acceleration and jerk are all parallel
  // only on the axis, so both methods must return the axis.
  for (const bool higherOrder : { false, true })
  {
    vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = constructGrid(10, 10, 10, Hexahedra);
    constructStretchedVortex(unstructuredGrid);

    vtkNew<vtkVortexCore> vortexCore;
    vortexCore->SetHigherOrderMethod(higherOrder);
    vortexCore->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
    vortexCore->SetInputData(unstructuredGrid);
    vortexCore->Update();

    vtkPolyData* lines = vortexCore->GetOutput();
    const char* label = higherOrder ? "higher order" : "Sujudi-Haimes";
    if (lines->GetNumberOfPoints() == 0)
    {
      std::cerr << "Stretched vortex, " << label << ": no core line" << std::endl;
      return EXIT_FAILURE;
    }
    double worst = 0.;
    for (vtkIdType i = 0; i < lines->GetNumberOfPoints(); ++i)
    {
      const double* p = lines->GetPoint(i);
      worst = std::max(worst, std::hypot(p[0] - 0.137, p[1] + 0.071));
    }
    if (worst > 0.05)
    {
      std::cerr << "Stretched vortex, " << label << ": core line point " << worst
                << " from the axis" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
