// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCurvatures.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkPolygon.h"
#include "vtkTriangle.h"
#include "vtkTriangleFilter.h"
#include "vtkTriangleStrip.h"

#include <memory> // For unique_ptr

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCurvatures);

//-------------------------------------------------------//
vtkCurvatures::vtkCurvatures()
{
  this->CurvatureType = VTK_CURVATURE_GAUSS;
  this->InvertMeanCurvature = 0;
}
//-------------------------------------------------------//
void vtkCurvatures::GetMeanCurvature(vtkPolyData* mesh)
{
  vtkDebugMacro("Start vtkCurvatures::GetMeanCurvature");

  bool hasTriangleStrip = false;
  for (vtkIdType cellId = 0; cellId < mesh->GetNumberOfCells(); ++cellId)
  {
    if (mesh->GetCellType(cellId) == VTK_TRIANGLE_STRIP)
    {
      hasTriangleStrip = true;
      break;
    }
  }

  vtkPolyData* polyData = mesh;
  vtkNew<vtkTriangleFilter> triangulateFilter;
  if (hasTriangleStrip)
  {
    triangulateFilter->SetInputData(mesh);
    triangulateFilter->Update();
    polyData = triangulateFilter->GetOutput();
  }

  // Empty array check
  if (polyData->GetNumberOfPolys() == 0 || polyData->GetNumberOfPoints() == 0)
  {
    vtkErrorMacro("No points/cells to operate on");
    return;
  }

  int numPts = polyData->GetNumberOfPoints();

  //     create-allocate
  const vtkNew<vtkIdList> vertices;
  const vtkNew<vtkIdList> vertices_n;
  const vtkNew<vtkIdList> neighbours;
  const vtkNew<vtkDoubleArray> meanCurvature;
  meanCurvature->SetName("Mean_Curvature");
  meanCurvature->SetNumberOfComponents(1);
  meanCurvature->SetNumberOfTuples(numPts);
  // Get the array so we can write to it directly
  double* meanCurvatureData = meanCurvature->GetPointer(0);

  //     create-allocate
  double n_f[3]; // normal of facet (could be stored for later?)
  double n_n[3]; // normal of edge
  double t[3];   // to store the cross product of n_f n_n
  double ore[3]; // origin of e
  double end[3]; // end of e
  double oth[3]; //     third vertex necessary for comp of n
  double vn0[3];
  double vn1[3]; // vertices for computation of neighbour's n
  double vn2[3];
  double e[3]; // edge (oriented)

  polyData->BuildLinks();
  // data init
  const int F = polyData->GetNumberOfCells();
  // init, preallocate the mean curvature
  const std::unique_ptr<int[]> num_neighb(new int[numPts]);
  for (int v = 0; v < numPts; v++)
  {
    meanCurvatureData[v] = 0.0;
    num_neighb[v] = 0;
  }

  //     main loop
  vtkDebugMacro(<< "Main loop: loop over facets such that id > id of neighb");
  vtkDebugMacro(<< "so that every edge comes only once");

  for (vtkIdType f = 0; f < F; ++f)
  {
    if (this->CheckAbort())
    {
      break;
    }
    polyData->GetCellPoints(f, vertices);
    const vtkIdType nv = vertices->GetNumberOfIds();

    for (vtkIdType v = 0; v < nv; v++)
    {
      // get neighbour
      const vtkIdType v_l = vertices->GetId(v);
      const vtkIdType v_r = vertices->GetId((v + 1) % nv);
      const vtkIdType v_o = vertices->GetId((v + 2) % nv);
      polyData->GetCellEdgeNeighbors(f, v_l, v_r, neighbours);

      vtkIdType n; // n short for neighbor

      // compute only if there is really ONE neighbour
      // AND meanCurvature has not been computed yet!
      // (ensured by n > f)
      if (neighbours->GetNumberOfIds() == 1 && (n = neighbours->GetId(0)) > f)
      {
        double Hf; // temporary store

        // find 3 corners of f: in order!
        polyData->GetPoint(v_l, ore);
        polyData->GetPoint(v_r, end);
        polyData->GetPoint(v_o, oth);
        // compute normal of f
        vtkTriangle::ComputeNormal(ore, end, oth, n_f);
        // compute common edge
        e[0] = end[0];
        e[1] = end[1];
        e[2] = end[2];
        e[0] -= ore[0];
        e[1] -= ore[1];
        e[2] -= ore[2];
        const double length = vtkMath::Normalize(e);
        double Af = vtkTriangle::TriangleArea(ore, end, oth);
        // find 3 corners of n: in order!
        polyData->GetCellPoints(n, vertices_n);
        polyData->GetPoint(vertices_n->GetId(0), vn0);
        polyData->GetPoint(vertices_n->GetId(1), vn1);
        polyData->GetPoint(vertices_n->GetId(2), vn2);
        Af += vtkTriangle::TriangleArea(vn0, vn1, vn2);
        // compute normal of n
        vtkTriangle::ComputeNormal(vn0, vn1, vn2, n_n);
        // the cosine is n_f * n_n
        const double cs = vtkMath::Dot(n_f, n_n);
        // the sin is (n_f x n_n) * e
        vtkMath::Cross(n_f, n_n, t);
        const double sn = vtkMath::Dot(t, e);
        // signed angle in [-pi,pi]
        if (sn != 0.0 || cs != 0.0)
        {
          const double angle = atan2(sn, cs);
          Hf = length * angle;
        }
        else
        {
          Hf = 0.0;
        }
        // add weighted Hf to scalar at v_l and v_r
        if (Af != 0.0)
        {
          (Hf /= Af) *= 3.0;
        }
        meanCurvatureData[v_l] += Hf;
        meanCurvatureData[v_r] += Hf;
        num_neighb[v_l] += 1;
        num_neighb[v_r] += 1;
      }
    }
  }

  // put curvature in vtkArray
  for (int v = 0; v < numPts; v++)
  {
    if (num_neighb[v] > 0)
    {
      const double Hf = 0.5 * meanCurvatureData[v] / num_neighb[v];
      if (this->InvertMeanCurvature)
      {
        meanCurvatureData[v] = -Hf;
      }
      else
      {
        meanCurvatureData[v] = Hf;
      }
    }
    else
    {
      meanCurvatureData[v] = 0.0;
    }
  }

  mesh->GetPointData()->AddArray(meanCurvature);
  mesh->GetPointData()->SetActiveScalars("Mean_Curvature");

  vtkDebugMacro("Set Values of Mean Curvature: Done");
}
//--------------------------------------------
#define CLAMP_MACRO(v) ((v) < (-1) ? (-1) : (v) > (1) ? (1) : (v))
void vtkCurvatures::GetGaussCurvature(vtkPolyData* output)
{
  vtkDebugMacro("Start vtkCurvatures::GetGaussCurvature()");
  // vtk data
  vtkCellArray* facets = output->GetPolys();

  vtkNew<vtkCellArray> triangleStrip;
  for (vtkIdType cellId = 0; cellId < output->GetNumberOfCells(); ++cellId)
  {
    if (this->CheckAbort())
    {
      break;
    }
    if (output->GetCellType(cellId) == VTK_TRIANGLE_STRIP)
    {
      vtkCell* cell = output->GetCell(cellId);
      vtkTriangleStrip::DecomposeStrip(
        cell->GetNumberOfPoints(), cell->GetPointIds()->GetPointer(0), triangleStrip);
    }
  }
  // Empty array check
  if ((triangleStrip->GetNumberOfCells() == 0 && output->GetNumberOfPolys() == 0) ||
    output->GetNumberOfPoints() == 0)
  {
    vtkErrorMacro("No points/cells to operate on");
    return;
  }

  int numPts = output->GetNumberOfPoints();
  const vtkNew<vtkDoubleArray> gaussCurvature;
  gaussCurvature->SetName("Gauss_Curvature");
  gaussCurvature->SetNumberOfComponents(1);
  gaussCurvature->SetNumberOfTuples(numPts);
  gaussCurvature->Fill(0.0);
  double* gaussCurvatureData = gaussCurvature->GetPointer(0);

  if (output->GetNumberOfPolys())
  {
    this->ComputeGaussCurvature(facets, output, gaussCurvatureData);
  }
  if (triangleStrip->GetNumberOfCells())
  {
    this->ComputeGaussCurvature(triangleStrip, output, gaussCurvatureData);
  }

  output->GetPointData()->AddArray(gaussCurvature);
  output->GetPointData()->SetActiveScalars("Gauss_Curvature");

  vtkDebugMacro("Set Values of Gauss Curvature: Done");
}

void vtkCurvatures::ComputeGaussCurvature(
  vtkCellArray* facets, vtkPolyData* output, double* gaussCurvatureData)
{
  double v0[3], v1[3], v2[3], e0[3], e1[3], e2[3];

  double A, alpha0, alpha1, alpha2;

  vtkIdType f;
  const vtkIdType* vert = nullptr;

  // other data
  vtkIdType Nv = output->GetNumberOfPoints();

  const std::unique_ptr<double[]> K(new double[Nv]);
  const std::unique_ptr<double[]> dA(new double[Nv]);
  double pi2 = 2.0 * vtkMath::Pi();
  for (int k = 0; k < Nv; k++)
  {
    K[k] = pi2;
    dA[k] = 0.0;
  }

  facets->InitTraversal();
  while (facets->GetNextCell(f, vert))
  {
    if (this->CheckAbort())
    {
      break;
    }
    output->GetPoint(vert[0], v0);
    output->GetPoint(vert[1], v1);
    output->GetPoint(vert[2], v2);
    // edges
    e0[0] = v1[0];
    e0[1] = v1[1];
    e0[2] = v1[2];
    e0[0] -= v0[0];
    e0[1] -= v0[1];
    e0[2] -= v0[2];

    e1[0] = v2[0];
    e1[1] = v2[1];
    e1[2] = v2[2];
    e1[0] -= v1[0];
    e1[1] -= v1[1];
    e1[2] -= v1[2];

    e2[0] = v0[0];
    e2[1] = v0[1];
    e2[2] = v0[2];
    e2[0] -= v2[0];
    e2[1] -= v2[1];
    e2[2] -= v2[2];

    alpha0 = vtkMath::Pi() - vtkMath::AngleBetweenVectors(e1, e2);
    alpha1 = vtkMath::Pi() - vtkMath::AngleBetweenVectors(e2, e0);
    alpha2 = vtkMath::Pi() - vtkMath::AngleBetweenVectors(e0, e1);

    // surf. area
    A = vtkTriangle::TriangleArea(v0, v1, v2);
    // UPDATE
    dA[vert[0]] += A;
    dA[vert[1]] += A;
    dA[vert[2]] += A;
    K[vert[0]] -= alpha1;
    K[vert[1]] -= alpha2;
    K[vert[2]] -= alpha0;
  }

  // put curvature in vtkArray
  for (int v = 0; v < Nv; v++)
  {
    if (dA[v] > 0.0)
    {
      gaussCurvatureData[v] = 3.0 * K[v] / dA[v];
    }
  }
}

void vtkCurvatures::GetMaximumCurvature(vtkPolyData* input, vtkPolyData* output)
{
  this->GetGaussCurvature(output);
  this->GetMeanCurvature(output);

  vtkIdType numPts = input->GetNumberOfPoints();

  const vtkNew<vtkDoubleArray> maximumCurvature;
  maximumCurvature->SetNumberOfComponents(1);
  maximumCurvature->SetNumberOfTuples(numPts);
  maximumCurvature->SetName("Maximum_Curvature");
  output->GetPointData()->AddArray(maximumCurvature);
  output->GetPointData()->SetActiveScalars("Maximum_Curvature");

  vtkDoubleArray* gauss =
    static_cast<vtkDoubleArray*>(output->GetPointData()->GetArray("Gauss_Curvature"));
  vtkDoubleArray* mean =
    static_cast<vtkDoubleArray*>(output->GetPointData()->GetArray("Mean_Curvature"));
  double k, h, k_max, tmp;

  for (vtkIdType i = 0; i < numPts; i++)
  {
    if (this->CheckAbort())
    {
      break;
    }

    k = gauss->GetComponent(i, 0);
    h = mean->GetComponent(i, 0);
    tmp = h * h - k;
    if (tmp >= 0)
    {
      k_max = h + sqrt(tmp);
    }
    else
    {
      k_max = h;
      if (tmp < -0.1)
      {
        vtkWarningMacro(
          << "The Gaussian or mean curvature at point " << i
          << " have a large computation error... The maximum curvature is likely off.");
      }
    }
    maximumCurvature->SetComponent(i, 0, k_max);
  }
}

void vtkCurvatures::GetMinimumCurvature(vtkPolyData* input, vtkPolyData* output)
{
  this->GetGaussCurvature(output);
  this->GetMeanCurvature(output);

  vtkIdType numPts = input->GetNumberOfPoints();

  const vtkNew<vtkDoubleArray> minimumCurvature;
  minimumCurvature->SetNumberOfComponents(1);
  minimumCurvature->SetNumberOfTuples(numPts);
  minimumCurvature->SetName("Minimum_Curvature");
  output->GetPointData()->AddArray(minimumCurvature);
  output->GetPointData()->SetActiveScalars("Minimum_Curvature");

  vtkDoubleArray* gauss =
    static_cast<vtkDoubleArray*>(output->GetPointData()->GetArray("Gauss_Curvature"));
  vtkDoubleArray* mean =
    static_cast<vtkDoubleArray*>(output->GetPointData()->GetArray("Mean_Curvature"));
  double k, h, k_min, tmp;

  for (vtkIdType i = 0; i < numPts; i++)
  {
    if (this->CheckAbort())
    {
      break;
    }

    k = gauss->GetComponent(i, 0);
    h = mean->GetComponent(i, 0);
    tmp = h * h - k;
    if (tmp >= 0)
    {
      k_min = h - sqrt(tmp);
    }
    else
    {
      k_min = h;
      if (tmp < -0.1)
      {
        vtkWarningMacro(
          << "The Gaussian or mean curvature at point " << i
          << " have a large computation error... The minimum curvature is likely off.");
      }
    }
    minimumCurvature->SetComponent(i, 0, k_min);
  }
}

//-------------------------------------------------------
int vtkCurvatures::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Null input check
  if (!input)
  {
    return 0;
  }

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());
  output->GetFieldData()->PassData(input->GetFieldData());

  //-------------------------------------------------------//
  //    Set Curvatures as PointData Scalars                //
  //-------------------------------------------------------//

  if (this->CurvatureType == VTK_CURVATURE_GAUSS)
  {
    this->GetGaussCurvature(output);
  }
  else if (this->CurvatureType == VTK_CURVATURE_MEAN)
  {
    this->GetMeanCurvature(output);
  }
  else if (this->CurvatureType == VTK_CURVATURE_MAXIMUM)
  {
    this->GetMaximumCurvature(input, output);
  }
  else if (this->CurvatureType == VTK_CURVATURE_MINIMUM)
  {
    this->GetMinimumCurvature(input, output);
  }
  else
  {
    vtkErrorMacro("Only Gauss, Mean, Max, and Min Curvature type available");
    return 1;
  }

  return 1;
}
/*-------------------------------------------------------*/
void vtkCurvatures::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CurvatureType: " << this->CurvatureType << "\n";
  os << indent << "InvertMeanCurvature: " << this->InvertMeanCurvature << "\n";
}
VTK_ABI_NAMESPACE_END
