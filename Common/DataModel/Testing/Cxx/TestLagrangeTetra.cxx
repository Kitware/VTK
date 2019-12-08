/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestInterpolationDerivs.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#define VTK_EPSILON 1e-10

#include "vtkLagrangeTetra.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include <vtkCellArray.h>
#include <vtkMinimalStandardRandomSequence.h>

#include "vtkClipDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

#define VISUAL_DEBUG 1

#ifdef VISUAL_DEBUG
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkVersion.h>
#endif

// #define DEBUG_POINTS

#ifdef DEBUG_POINTS
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkXMLPolyDataWriter.h>
#endif

#include <cassert>
#include <cmath>
#include <vector>

namespace
{
vtkSmartPointer<vtkLagrangeTetra> CreateTetra(int nPoints)
{
  vtkSmartPointer<vtkLagrangeTetra> t = vtkSmartPointer<vtkLagrangeTetra>::New();

  t->GetPointIds()->SetNumberOfIds(nPoints);
  t->GetPoints()->SetNumberOfPoints(nPoints);
  t->Initialize();
  double* points = t->GetParametricCoords();
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    t->GetPointIds()->SetId(i, i);
    t->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return t;
}

int TestInterpolationFunction(vtkSmartPointer<vtkLagrangeTetra> cell, double eps = VTK_EPSILON)
{
  int numPts = cell->GetNumberOfPoints();
  std::vector<double> sf(numPts);
  double* coords = cell->GetParametricCoords();
  int r = 0;
  for (int i = 0; i < numPts; ++i)
  {
    double* point = coords + 3 * i;
    double sum = 0.;
    cell->InterpolateFunctions(point, sf.data()); // virtual function
    for (int j = 0; j < numPts; j++)
    {
      sum += sf[j];
      if (j == i)
      {
        if (fabs(sf[j] - 1) > eps)
        {
          std::cout << "fabs(sf[" << j << "] - 1): " << fabs(sf[j] - 1) << std::endl;
          ++r;
        }
      }
      else
      {
        if (fabs(sf[j] - 0) > eps)
        {
          std::cout << "fabs(sf[" << j << "] - 0): " << fabs(sf[j] - 0) << std::endl;
          ++r;
        }
      }
    }
    if (fabs(sum - 1) > eps)
    {
      std::cout << "fabs(" << sum << " - 1): " << fabs(sum - 1) << std::endl;
      ++r;
    }
  }

  // Let's test unity condition on the center point:
  double center[3];
  cell->GetParametricCenter(center);
  cell->InterpolateFunctions(center, sf.data()); // virtual function
  double sum = 0.;
  for (int j = 0; j < numPts; j++)
  {
    sum += sf[j];
  }
  if (fabs(sum - 1) > eps)
  {
    std::cout << "center: fabs(" << sum << " - 1): " << fabs(sum - 1) << std::endl;
    ++r;
  }

  return r;
}

void InterpolateDerivsNumeric(vtkSmartPointer<vtkLagrangeTetra> cell, double pcoords[3],
  double* derivs, double eps = VTK_EPSILON)
{
  vtkIdType nPoints = cell->GetPoints()->GetNumberOfPoints();
  std::vector<double> valm(nPoints);
  std::vector<double> valp(nPoints);

  double pcoordsp[3], pcoordsm[3];

  for (vtkIdType i = 0; i < 3; i++)
  {
    pcoordsp[i] = pcoordsm[i] = pcoords[i];
  }
  pcoordsp[0] += eps;
  cell->InterpolateFunctions(pcoordsp, valp.data());

  pcoordsm[0] -= eps;
  cell->InterpolateFunctions(pcoordsm, valm.data());

  for (vtkIdType idx = 0; idx < nPoints; idx++)
  {
    derivs[idx] = (valp[idx] - valm[idx]) / (2. * eps);
  }

  for (vtkIdType i = 0; i < 3; i++)
  {
    pcoordsp[i] = pcoordsm[i] = pcoords[i];
  }
  pcoordsp[1] += eps;
  cell->InterpolateFunctions(pcoordsp, valp.data());

  pcoordsm[1] -= eps;
  cell->InterpolateFunctions(pcoordsm, valm.data());

  for (vtkIdType idx = 0; idx < nPoints; idx++)
  {
    derivs[nPoints + idx] = (valp[idx] - valm[idx]) / (2. * eps);
  }

  if (cell->GetCellDimension() == 3)
  {
    for (vtkIdType i = 0; i < 3; i++)
    {
      pcoordsp[i] = pcoordsm[i] = pcoords[i];
    }
    pcoordsp[2] += eps;
    cell->InterpolateFunctions(pcoordsp, valp.data());

    pcoordsm[2] -= eps;
    cell->InterpolateFunctions(pcoordsm, valm.data());

    for (vtkIdType idx = 0; idx < nPoints; idx++)
    {
      derivs[2 * nPoints + idx] = (valp[idx] - valm[idx]) / (2. * eps);
    }
  }
}

int TestInterpolationDerivs(vtkSmartPointer<vtkLagrangeTetra> cell, double eps = VTK_EPSILON)
{
  int numPts = cell->GetNumberOfPoints();
  int dim = cell->GetCellDimension();
  std::vector<double> derivs(dim * numPts);
  std::vector<double> derivs_n(dim * numPts);
  double* coords = cell->GetParametricCoords();
  int r = 0;
  for (int i = 0; i < numPts; ++i)
  {
    double* point = coords + 3 * i;
    double sum = 0.;
    cell->InterpolateDerivs(point, derivs.data());
    InterpolateDerivsNumeric(cell, point, derivs_n.data(), 1.e-10);
    for (int j = 0; j < dim * numPts; j++)
    {
      sum += derivs[j];
      if (fabs(derivs[j] - derivs_n[j]) >
        1.e-5 * (fabs(derivs[j]) > numPts ? fabs(derivs[j]) : numPts))
      {
        std::cout << j << " is different from numeric! " << derivs[j] << " " << derivs_n[j] << " "
                  << fabs(derivs[j] - derivs_n[j]) << std::endl;
        ++r;
      }
    }
    if (fabs(sum) > eps * numPts)
    {
      std::cout << "nonzero! " << sum << std::endl;
      ++r;
    }
  }

  // Let's test zero condition on the center point:
  double center[3];
  cell->GetParametricCenter(center);
  cell->InterpolateDerivs(center, derivs.data());
  double sum = 0.;
  for (int j = 0; j < dim * numPts; j++)
  {
    sum += derivs[j];
  }
  if (fabs(sum) > eps)
  {
    std::cout << "center: nonzero!" << std::endl;
    ++r;
  }

  return r;
}

void ViewportRange(int testNum, double* range)
{
  range[0] = .25 * (testNum % 4);
  range[1] = range[0] + .25;
  range[2] = .25 * (testNum / 4);
  range[3] = range[2] + .25;
}

void RandomCircle(
  vtkMinimalStandardRandomSequence* sequence, double radius, double* offset, double* value)
{
  double theta = 2. * vtkMath::Pi() * sequence->GetValue();
  sequence->Next();
  value[0] = radius * cos(theta) + offset[0];
  value[1] = radius * sin(theta) + offset[1];
}

void RandomSphere(
  vtkMinimalStandardRandomSequence* sequence, double radius, double* offset, double* value)
{
  double theta = 2. * vtkMath::Pi() * sequence->GetValue();
  sequence->Next();
  double phi = vtkMath::Pi() * sequence->GetValue();
  sequence->Next();
  value[0] = radius * cos(theta) * sin(phi) + offset[0];
  value[1] = radius * sin(theta) * sin(phi) + offset[1];
  value[2] = radius * cos(phi) + offset[2];
}

static int testNum = 0;

vtkIdType IntersectWithCell(unsigned nTest, vtkMinimalStandardRandomSequence* sequence,
  bool threeDimensional, double radius, double* offset, vtkCell* cell
#ifdef VISUAL_DEBUG
  ,
  vtkSmartPointer<vtkRenderWindow> renderWindow
#endif
)
{
  double p[2][3];
  p[0][2] = p[1][2] = 0.;
  double tol = 1.e-7;
  double t;
  double intersect[3];
  double pcoords[3];
  int subId;
  vtkIdType counter = 0;

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();

  for (unsigned i = 0; i < nTest; i++)
  {
    if (threeDimensional)
    {
      RandomSphere(sequence, radius, offset, p[0]);
      RandomSphere(sequence, radius, offset, p[1]);
    }
    else
    {
      RandomCircle(sequence, radius, offset, p[0]);
      RandomCircle(sequence, radius, offset, p[1]);
    }

    if (cell->IntersectWithLine(p[0], p[1], tol, t, intersect, pcoords, subId))
    {
      counter++;
      vtkIdType pid = points->InsertNextPoint(intersect);
      vertices->InsertNextCell(1, &pid);
    }
  }

#ifdef DEBUG_POINTS
  {
    vtkSmartPointer<vtkLagrangeTetra> tet = CreateTetra(10);
    vtkNew<vtkPolyData> pts;
    pts->SetPoints(points);
    pts->SetVerts(vertices);
    vtkNew<vtkXMLPolyDataWriter> w;
    w->SetFileName("intersection.vtp");
    w->SetInputData(pts.GetPointer());
    w->Write();
  }
#endif

#ifdef VISUAL_DEBUG
  vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
  camera->SetPosition(2, 2, 2);
  camera->SetFocalPoint(offset[0], offset[1], offset[2]);

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->SetActiveCamera(camera);
  renderWindow->AddRenderer(renderer);
  double dim[4];
  ViewportRange(testNum++, dim);
  renderer->SetViewport(dim[0], dim[2], dim[1], dim[3]);

  vtkSmartPointer<vtkPolyData> point = vtkSmartPointer<vtkPolyData>::New();

  point->SetPoints(points);
  point->SetVerts(vertices);

  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(point);

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);
  renderer->ResetCamera();

  renderWindow->Render();
#endif

  return counter;
}

vtkIdType TestClip(vtkCell* cell
#ifdef VISUAL_DEBUG
  ,
  vtkSmartPointer<vtkRenderWindow> renderWindow
#endif
)

{
  vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(cell->GetPoints());

  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();
  cellArray->InsertNextCell(cell);
  unstructuredGrid->SetCells(cell->GetCellType(), cellArray);

  vtkSmartPointer<vtkDoubleArray> radiant = vtkSmartPointer<vtkDoubleArray>::New();
  radiant->SetName("Distance from Origin");
  radiant->SetNumberOfTuples(cell->GetPointIds()->GetNumberOfIds());

  double maxDist = 0.;
  for (vtkIdType i = 0; i < cell->GetPointIds()->GetNumberOfIds(); i++)
  {
    double xyz[3];
    cell->GetPoints()->GetPoint(i, xyz);
    double dist = sqrt(xyz[0] * xyz[0] + xyz[1] * xyz[1] + xyz[2] * xyz[2]);
    radiant->SetTypedTuple(i, &dist);
    maxDist = (dist > maxDist ? dist : maxDist);
  }

  unstructuredGrid->GetPointData()->AddArray(radiant);
  unstructuredGrid->GetPointData()->SetScalars(radiant);

  vtkSmartPointer<vtkClipDataSet> clip = vtkSmartPointer<vtkClipDataSet>::New();
  clip->SetValue(maxDist * .5);
  clip->SetInputData(unstructuredGrid);

  vtkSmartPointer<vtkDataSetSurfaceFilter> surfaceFilter =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
  surfaceFilter->SetInputConnection(clip->GetOutputPort());
  surfaceFilter->Update();
  vtkPolyData* polydata = surfaceFilter->GetOutput();

#ifdef VISUAL_DEBUG
  vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
  camera->SetPosition(-2. * maxDist, -2. * maxDist, -2. * maxDist);
  camera->SetFocalPoint(0, 0, 0);

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->SetActiveCamera(camera);
  renderWindow->AddRenderer(renderer);
  double dim[4];
  ViewportRange(testNum++, dim);
  renderer->SetViewport(dim[0], dim[2], dim[1], dim[3]);

  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(polydata);
  mapper->SetScalarRange(maxDist * .5, maxDist);

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  renderWindow->Render();
#endif

  return polydata->GetNumberOfPoints();
}
}

int TestLagrangeTetra(int argc, char* argv[])
{
#ifdef VISUAL_DEBUG
  vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->SetSize(500, 500);

  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();

  renderWindowInteractor->SetRenderWindow(renderWindow);
#endif

  int r = 0;

  // empirically determined values, verified visually
  static const vtkIdType nIntersections = 50;
  static const vtkIdType nClippedElems[11] = { 0, 6, 9, 33, 45, 85, 91, 26 };

  vtkIdType nPointsForOrder[8] = { -1, 4, 10, 20, 35, 56, 84, 15 };

  for (vtkIdType order = 1; order <= 7; ++order)
  {
    vtkSmartPointer<vtkLagrangeTetra> t = CreateTetra(nPointsForOrder[order]);

#ifdef DEBUG_POINTS
    {
      vtkNew<vtkPolyData> p;
      p->SetPoints(t->GetPoints());
      vtkNew<vtkXMLPolyDataWriter> w;
      w->SetFileName("test.vtp");
      w->SetInputData(p.GetPointer());
      w->Write();
    }
#endif

    if (t->GetPoints()->GetNumberOfPoints() != 15)
    {
      for (vtkIdType i = 0; i < t->GetPoints()->GetNumberOfPoints(); i++)
      {
        vtkIdType bindex[4] = { static_cast<vtkIdType>(t->GetPoints()->GetPoint(i)[0] * order + .5),
          static_cast<vtkIdType>(t->GetPoints()->GetPoint(i)[1] * order + .5),
          static_cast<vtkIdType>(t->GetPoints()->GetPoint(i)[2] * order + .5), 0 };
        bindex[3] = order - bindex[0] - bindex[1] - bindex[2];
        vtkIdType idx = t->ToIndex(bindex);
        if (i != idx)
        {
          std::cout << "index mismatch for order " << order << "! " << i << " " << idx << std::endl;
          std::cout << "bindex: " << bindex[0] << " " << bindex[1] << " " << bindex[2] << " "
                    << bindex[3] << std::endl;
          std::cout << "There are " << t->GetPoints()->GetNumberOfPoints() << " points"
                    << std::endl;
          return 1;
        }
        vtkIdType bindex_[4];
        t->ToBarycentricIndex(i, bindex_);
        if (bindex[0] != bindex_[0] || bindex[1] != bindex_[1] || bindex[2] != bindex_[2] ||
          bindex[3] != bindex_[3])
        {
          std::cout << "barycentric index mismatch for order " << order << ", index " << i << "! ("
                    << bindex[0] << " " << bindex[1] << " " << bindex[2] << " " << bindex[3]
                    << ") vs " << bindex_[0] << " " << bindex_[1] << " " << bindex_[2] << " "
                    << bindex_[3] << ")" << std::endl;
          return 1;
        }
      }
    }

    r += TestInterpolationFunction(t);
    if (r)
    {
      std::cout << "Order " << order << " function failed!" << std::endl;
      break;
    }
    r += TestInterpolationDerivs(t);
    if (r)
    {
      std::cout << "Order " << order << " derivs failed!" << std::endl;
      break;
    }

    {
      vtkMinimalStandardRandomSequence* sequence = vtkMinimalStandardRandomSequence::New();

      sequence->SetSeed(1);

      unsigned nTest = 5.e2;
      double radius = 1.5;
      double center[3] = { 0.5, 0.5, 0. };
      vtkIdType nHits = IntersectWithCell(nTest, sequence, true, radius, center, t
#ifdef VISUAL_DEBUG
        ,
        renderWindow
#endif
      );

      sequence->Delete();

      r += (nHits == nIntersections) ? 0 : 1;

      if (r)
      {
        std::cout << "Order " << order << " intersection failed!" << std::endl;
        break;
      }
    }

    {
      vtkIdType nClippedElements = TestClip(t
#ifdef VISUAL_DEBUG
        ,
        renderWindow
#endif
      );
      r += (nClippedElements == nClippedElems[order]) ? 0 : 1;

      if (r)
      {
        std::cout << "Order " << order << " clip failed!" << std::endl;
        break;
      }
    }
  }

#ifdef VISUAL_DEBUG
  for (vtkIdType i = testNum; i < 16; i++)
  {
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    renderWindow->AddRenderer(renderer);
    double dim[4];
    ViewportRange(testNum++, dim);
    renderer->SetViewport(dim[0], dim[2], dim[1], dim[3]);
    renderer->SetBackground(0., 0., 0.);
  }

  renderWindowInteractor->Initialize();

  int retVal = vtkRegressionTestImage(renderWindow.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  r += (retVal == vtkRegressionTester::PASSED) ? 0 : 1;
#endif

  return r;
}
