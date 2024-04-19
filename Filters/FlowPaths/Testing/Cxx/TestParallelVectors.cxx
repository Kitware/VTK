// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkHexahedron.h"
#include "vtkParallelVectors.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTetra.h"
#include "vtkUnstructuredGrid.h"

#include "vtkXMLPolyDataWriter.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"

#ifndef M_PI
#include "vtkMath.h"
#define M_PI vtkMath::Pi()
#endif

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

void constructFieldProfile(vtkUnstructuredGrid* unstructuredGrid)
{
  const double z0 = -1.5;

  auto f = [=](const double& t) -> std::array<double, 2> {
    const double amplitude = .8;
    const double phase = 2.;

    return { amplitude * cos(2. * M_PI * t / phase), amplitude * sin(2. * M_PI * t / phase) };
  };

  auto v = [=](const double p[3]) -> std::array<double, 3> {
    const double& x = p[0];
    const double& y = p[1];
    const double& z = p[2];

    const double t = (z - z0);
    const std::array<double, 2> f_t = f(t);

    return { x - f_t[0], y - f_t[1], t };
  };

  auto w = [=](const double p[3]) -> std::array<double, 3> {
    const double& x = p[0];
    const double& y = p[1];
    const double& z = p[2];

    const double t = (z - z0);
    const std::array<double, 2> f_t = f(t);

    return { f_t[0] - x, f_t[1] - y, t };
  };

  vtkPoints* pointArray = unstructuredGrid->GetPoints();

  vtkNew<vtkDoubleArray> vField;
  vField->SetName("vField");
  vField->SetNumberOfComponents(3);
  vField->SetNumberOfTuples(pointArray->GetNumberOfPoints());
  for (int i = 0; i < pointArray->GetNumberOfPoints(); ++i)
  {
    vField->SetTuple(i, v(pointArray->GetPoint(i)).data());
  }
  unstructuredGrid->GetPointData()->AddArray(vField);

  vtkNew<vtkDoubleArray> wField;
  wField->SetName("wField");
  wField->SetNumberOfComponents(3);
  wField->SetNumberOfTuples(pointArray->GetNumberOfPoints());
  for (int i = 0; i < pointArray->GetNumberOfPoints(); ++i)
  {
    wField->SetTuple(i, w(pointArray->GetPoint(i)).data());
  }
  unstructuredGrid->GetPointData()->AddArray(wField);
}
}

int TestParallelVectors(int argc, char* argv[])
{
  vtkSmartPointer<vtkPolyData> output1;
  {
    vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = constructGrid(5, 5, 5, Hexahedra);
    constructFieldProfile(unstructuredGrid);

    vtkNew<vtkParallelVectors> parallelVectors;
    parallelVectors->SetInputData(unstructuredGrid);
    parallelVectors->SetFirstVectorFieldName("vField");
    parallelVectors->SetSecondVectorFieldName("wField");
    parallelVectors->Update();

    output1 = vtkPolyData::SafeDownCast(parallelVectors->GetOutput());
  }

  vtkSmartPointer<vtkPolyData> output2;
  {
    vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = constructGrid(5, 5, 5, Tetrahedra);

    constructFieldProfile(unstructuredGrid);

    vtkNew<vtkParallelVectors> parallelVectors;
    parallelVectors->SetInputData(unstructuredGrid);
    parallelVectors->SetFirstVectorFieldName("wField");
    parallelVectors->SetSecondVectorFieldName("vField");
    parallelVectors->Update();

    output2 = vtkPolyData::SafeDownCast(parallelVectors->GetOutput());
  }

  if (output1->GetNumberOfCells() != output2->GetNumberOfCells())
  {
    return EXIT_FAILURE;
  }

  auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(output1);
  mapper->ScalarVisibilityOff();

  auto actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(0, 0, 0);
  actor->GetProperty()->SetLineWidth(1.);
  actor->SetPosition(0, 0, 1);

  auto renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->AddActor(actor);
  renderer->ResetCamera();
  renderer->SetBackground(1., 1., 1.);

  auto renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);

  auto iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
