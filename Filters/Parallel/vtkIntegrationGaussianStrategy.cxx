// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIntegrationGaussianStrategy.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformationQuadratureSchemeDefinitionVectorKey.h"
#include "vtkIntegrateAttributes.h"
#include "vtkIntegrateAttributesFieldList.h"
#include "vtkIntegrationLinearStrategy.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkQuadraturePointInterpolator.h"
#include "vtkQuadratureSchemeDefinition.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{
// Those tables contain all the numbers necessary for proper Gauss-Legendre quadrature integration.
// For each supported cell type, a table representing the shape function, the quadrature weights
// and the shape function derivative is provided. For performance purposes, the functions are
// precomputed on the quadrature points.

// Reference element : [-1,1]^2 quad
constexpr double SFW_QUAD_BILINEAR[] = { 6.22008467928145e-01, 1.66666666666667e-01,
  4.46581987385206e-02, 1.66666666666667e-01, 1.66666666666667e-01, 4.46581987385206e-02,
  1.66666666666667e-01, 6.22008467928145e-01, 1.66666666666667e-01, 6.22008467928145e-01,
  1.66666666666667e-01, 4.46581987385206e-02, 4.46581987385206e-02, 1.66666666666667e-01,
  6.22008467928145e-01, 1.66666666666667e-01 };

constexpr double QW_QUAD_BILINEAR[] = { 1.0, 1.0, 1.0, 1.0 };

constexpr double SFDW_QUAD_BILINEAR[] = { -0.39433756729740643, 0.39433756729740643,
  0.10566243270259354, -0.10566243270259354, -0.39433756729740643, -0.10566243270259354,
  0.10566243270259354, 0.39433756729740643,

  -0.10566243270259354, 0.10566243270259354, 0.39433756729740643, -0.39433756729740643,
  -0.39433756729740643, -0.10566243270259354, 0.10566243270259354, 0.39433756729740643,

  -0.39433756729740643, 0.39433756729740643, 0.10566243270259354, -0.10566243270259354,
  -0.10566243270259354, -0.39433756729740643, 0.39433756729740643, 0.10566243270259354,

  -0.10566243270259354, 0.10566243270259354, 0.39433756729740643, -0.39433756729740643,
  -0.10566243270259354, -0.39433756729740643, 0.39433756729740643, 0.10566243270259354 };

// Reference element : [-1,1]^3 hex
constexpr double SFW_HEX_TRILINEAR[] = { 0.490562612162344, 0.131445855765802, 0.0352208109008645,
  0.131445855765802, 0.131445855765802, 0.0352208109008645, 0.00943738783765593, 0.0352208109008645,

  0.131445855765802, 0.0352208109008645, 0.00943738783765593, 0.0352208109008645, 0.490562612162344,
  0.131445855765802, 0.0352208109008645, 0.131445855765802,

  0.131445855765802, 0.0352208109008645, 0.131445855765802, 0.490562612162344, 0.0352208109008645,
  0.00943738783765593, 0.0352208109008645, 0.131445855765802,

  0.0352208109008645, 0.00943738783765593, 0.0352208109008645, 0.131445855765802, 0.131445855765802,
  0.0352208109008645, 0.131445855765802, 0.490562612162344,

  0.131445855765802, 0.490562612162344, 0.131445855765802, 0.0352208109008645, 0.0352208109008645,
  0.131445855765802, 0.0352208109008645, 0.00943738783765593,

  0.0352208109008645, 0.131445855765802, 0.0352208109008645, 0.00943738783765593, 0.131445855765802,
  0.490562612162344, 0.131445855765802, 0.0352208109008645,

  0.0352208109008645, 0.131445855765802, 0.490562612162344, 0.131445855765802, 0.00943738783765593,
  0.0352208109008645, 0.131445855765802, 0.0352208109008645,

  0.00943738783765593, 0.0352208109008645, 0.131445855765802, 0.0352208109008645,
  0.0352208109008645, 0.131445855765802, 0.490562612162344, 0.131445855765802 };

constexpr double QW_HEX_TRILINEAR[] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };

constexpr double SFDW_HEX_TRILINEAR[] = { -0.311004233964073, 0.311004233964073, 0.0833333333333333,
  -0.0833333333333333, -0.0833333333333333, 0.0833333333333333, 0.0223290993692602,
  -0.0223290993692602, -0.311004233964073, -0.0833333333333333, 0.0833333333333333,
  0.311004233964073, -0.0833333333333333, -0.0223290993692602, 0.0223290993692602,
  0.0833333333333333, -0.311004233964073, -0.0833333333333333, -0.0223290993692602,
  -0.0833333333333333, 0.311004233964073, 0.0833333333333333, 0.0223290993692602,
  0.0833333333333333,

  -0.0833333333333333, 0.0833333333333333, 0.0223290993692602, -0.0223290993692602,
  -0.311004233964073, 0.311004233964073, 0.0833333333333333, -0.0833333333333333,
  -0.0833333333333333, -0.0223290993692602, 0.0223290993692602, 0.0833333333333333,
  -0.311004233964073, -0.0833333333333333, 0.0833333333333333, 0.311004233964073,
  -0.311004233964073, -0.0833333333333333, -0.0223290993692602, -0.0833333333333333,
  0.311004233964073, 0.0833333333333333, 0.0223290993692602, 0.0833333333333333,

  -0.0833333333333333, 0.0833333333333333, 0.311004233964073, -0.311004233964073,
  -0.0223290993692602, 0.0223290993692602, 0.0833333333333333, -0.0833333333333333,
  -0.311004233964073, -0.0833333333333333, 0.0833333333333333, 0.311004233964073,
  -0.0833333333333333, -0.0223290993692602, 0.0223290993692602, 0.0833333333333333,
  -0.0833333333333333, -0.0223290993692602, -0.0833333333333333, -0.311004233964073,
  0.0833333333333333, 0.0223290993692602, 0.0833333333333333, 0.311004233964073,

  -0.0223290993692602, 0.0223290993692602, 0.0833333333333333, -0.0833333333333333,
  -0.0833333333333333, 0.0833333333333333, 0.311004233964073, -0.311004233964073,
  -0.0833333333333333, -0.0223290993692602, 0.0223290993692602, 0.0833333333333333,
  -0.311004233964073, -0.0833333333333333, 0.0833333333333333, 0.311004233964073,
  -0.0833333333333333, -0.0223290993692602, -0.0833333333333333, -0.311004233964073,
  0.0833333333333333, 0.0223290993692602, 0.0833333333333333, 0.311004233964073,

  -0.311004233964073, 0.311004233964073, 0.0833333333333333, -0.0833333333333333,
  -0.0833333333333333, 0.0833333333333333, 0.0223290993692602, -0.0223290993692602,
  -0.0833333333333333, -0.311004233964073, 0.311004233964073, 0.0833333333333333,
  -0.0223290993692602, -0.0833333333333333, 0.0833333333333333, 0.0223290993692602,
  -0.0833333333333333, -0.311004233964073, -0.0833333333333333, -0.0223290993692602,
  0.0833333333333333, 0.311004233964073, 0.0833333333333333, 0.0223290993692602,

  -0.0833333333333333, 0.0833333333333333, 0.0223290993692602, -0.0223290993692602,
  -0.311004233964073, 0.311004233964073, 0.0833333333333333, -0.0833333333333333,
  -0.0223290993692602, -0.0833333333333333, 0.0833333333333333, 0.0223290993692602,
  -0.0833333333333333, -0.311004233964073, 0.311004233964073, 0.0833333333333333,
  -0.0833333333333333, -0.311004233964073, -0.0833333333333333, -0.0223290993692602,
  0.0833333333333333, 0.311004233964073, 0.0833333333333333, 0.0223290993692602,

  -0.0833333333333333, 0.0833333333333333, 0.311004233964073, -0.311004233964073,
  -0.0223290993692602, 0.0223290993692602, 0.0833333333333333, -0.0833333333333333,
  -0.0833333333333333, -0.311004233964073, 0.311004233964073, 0.0833333333333333,
  -0.0223290993692602, -0.0833333333333333, 0.0833333333333333, 0.0223290993692602,
  -0.0223290993692602, -0.0833333333333333, -0.311004233964073, -0.0833333333333333,
  0.0223290993692602, 0.0833333333333333, 0.311004233964073, 0.0833333333333333,

  -0.0223290993692602, 0.0223290993692602, 0.0833333333333333, -0.0833333333333333,
  -0.0833333333333333, 0.0833333333333333, 0.311004233964073, -0.311004233964073,
  -0.0223290993692602, -0.0833333333333333, 0.0833333333333333, 0.0223290993692602,
  -0.0833333333333333, -0.311004233964073, 0.311004233964073, 0.0833333333333333,
  -0.0223290993692602, -0.0833333333333333, -0.311004233964073, -0.0833333333333333,
  0.0223290993692602, 0.0833333333333333, 0.311004233964073, 0.0833333333333333 };

// Reference element : x in [0,1], y in [0,1], z in [0,1], x + y < 1
constexpr double SFW_WEDGE_TRILINEAR[] = { 0.525783423063209, 0.131445855765802, 0.131445855765802,
  0.140883243603458, 0.0352208109008645, 0.0352208109008645,

  0.140883243603458, 0.0352208109008645, 0.0352208109008645, 0.525783423063209, 0.131445855765802,
  0.131445855765802,

  0.131445855765802, 0.131445855765802, 0.525783423063209, 0.0352208109008645, 0.0352208109008645,
  0.140883243603458,

  0.0352208109008645, 0.0352208109008645, 0.140883243603458, 0.131445855765802, 0.131445855765802,
  0.525783423063209,

  0.131445855765802, 0.525783423063209, 0.131445855765802, 0.0352208109008645, 0.140883243603458,
  0.0352208109008645,

  0.0352208109008645, 0.140883243603458, 0.0352208109008645, 0.131445855765802, 0.525783423063209,
  0.131445855765802 };

constexpr double QW_WEDGE_TRILINEAR[] = { 0.083333333333333333, 0.083333333333333333,
  0.083333333333333333, 0.083333333333333333, 0.083333333333333333, 0.083333333333333333 };

constexpr double SFDW_WEDGE_TRILINEAR[] = { -0.788675134594813, 0, 0.788675134594813,
  -0.211324865405187, 0, 0.211324865405187, -0.788675134594813, 0.788675134594813, 0,
  -0.211324865405187, 0.211324865405187, 0, -0.666666666666667, -0.166666666666667,
  -0.166666666666667, 0.666666666666667, 0.166666666666667, 0.166666666666667,

  -0.211324865405187, 0, 0.211324865405187, -0.788675134594813, 0, 0.788675134594813,
  -0.211324865405187, 0.211324865405187, 0, -0.788675134594813, 0.788675134594813, 0,
  -0.666666666666667, -0.166666666666667, -0.166666666666667, 0.666666666666667, 0.166666666666667,
  0.166666666666667,

  -0.788675134594813, 0, 0.788675134594813, -0.211324865405187, 0, 0.211324865405187,
  -0.788675134594813, 0.788675134594813, 0, -0.211324865405187, 0.211324865405187, 0,
  -0.166666666666667, -0.166666666666667, -0.666666666666667, 0.166666666666667, 0.166666666666667,
  0.666666666666667,

  -0.211324865405187, 0, 0.211324865405187, -0.788675134594813, 0, 0.788675134594813,
  -0.211324865405187, 0.211324865405187, 0, -0.788675134594813, 0.788675134594813, 0,
  -0.166666666666667, -0.166666666666667, -0.666666666666667, 0.166666666666667, 0.166666666666667,
  0.666666666666667,

  -0.788675134594813, 0, 0.788675134594813, -0.211324865405187, 0, 0.211324865405187,
  -0.788675134594813, 0.788675134594813, 0, -0.211324865405187, 0.211324865405187, 0,
  -0.166666666666667, -0.666666666666667, -0.166666666666667, 0.166666666666667, 0.666666666666667,
  0.166666666666667,

  -0.211324865405187, 0, 0.211324865405187, -0.788675134594813, 0, 0.788675134594813,
  -0.211324865405187, 0.211324865405187, 0, -0.788675134594813, 0.788675134594813, 0,
  -0.166666666666667, -0.666666666666667, -0.166666666666667, 0.166666666666667, 0.666666666666667,
  0.166666666666667 };

// Reference element : base plane [-1,1]^2, apex (0,0,1)
constexpr double SFW_PYR_TRILINEAR[] = { 0.490562612162344, 0.131445855765802, 0.0352208109008645,
  0.131445855765802, 0.211324865405187,

  0.131445855765802, 0.0352208109008645, 0.00943738783765592, 0.0352208109008645, 0.788675134594813,

  0.131445855765802, 0.0352208109008645, 0.131445855765802, 0.490562612162344, 0.211324865405187,

  0.0352208109008645, 0.00943738783765592, 0.0352208109008645, 0.131445855765802, 0.788675134594813,

  0.131445855765802, 0.490562612162344, 0.131445855765802, 0.0352208109008645, 0.211324865405187,

  0.0352208109008645, 0.131445855765802, 0.0352208109008645, 0.00943738783765592, 0.788675134594813,

  0.0352208109008645, 0.131445855765802, 0.490562612162344, 0.131445855765802, 0.211324865405187,

  0.00943738783765592, 0.0352208109008645, 0.131445855765802, 0.0352208109008645,
  0.788675134594813 };

constexpr double QW_PYR_TRILINEAR[] = { 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5 };

constexpr double SFDW_PYR_TRILINEAR[] = { -0.311004233964073, 0.311004233964073, 0.0833333333333333,
  -0.0833333333333333, 0, -0.311004233964073, -0.0833333333333333, 0.0833333333333333,
  0.311004233964073, 0, -0.622008467928146, -0.166666666666667, -0.0446581987385204,
  -0.166666666666667, 1,

  -0.0833333333333333, 0.0833333333333333, 0.0223290993692602, -0.0223290993692602, 0,
  -0.0833333333333333, -0.0223290993692602, 0.0223290993692602, 0.0833333333333333, 0,
  -0.622008467928146, -0.166666666666667, -0.0446581987385204, -0.166666666666667, 1,

  -0.0833333333333333, 0.0833333333333333, 0.311004233964073, -0.311004233964073, 0,
  -0.311004233964073, -0.0833333333333333, 0.0833333333333333, 0.311004233964073, 0,
  -0.166666666666667, -0.0446581987385204, -0.166666666666667, -0.622008467928146, 1,

  -0.0223290993692602, 0.0223290993692602, 0.0833333333333333, -0.0833333333333333, 0,
  -0.0833333333333333, -0.0223290993692602, 0.0223290993692602, 0.0833333333333333, 0,
  -0.166666666666667, -0.0446581987385204, -0.166666666666667, -0.622008467928146, 1,

  -0.311004233964073, 0.311004233964073, 0.0833333333333333, -0.0833333333333333, 0,
  -0.0833333333333333, -0.311004233964073, 0.311004233964073, 0.0833333333333333, 0,
  -0.166666666666667, -0.622008467928146, -0.166666666666667, -0.0446581987385204, 1,

  -0.0833333333333333, 0.0833333333333333, 0.0223290993692602, -0.0223290993692602, 0,
  -0.0223290993692602, -0.0833333333333333, 0.0833333333333333, 0.0223290993692602, 0,
  -0.166666666666667, -0.622008467928146, -0.166666666666667, -0.0446581987385204, 1,

  -0.0833333333333333, 0.0833333333333333, 0.311004233964073, -0.311004233964073, 0,
  -0.0833333333333333, -0.311004233964073, 0.311004233964073, 0.0833333333333333, 0,
  -0.0446581987385204, -0.166666666666667, -0.622008467928146, -0.166666666666667, 1,

  -0.0223290993692602, 0.0223290993692602, 0.0833333333333333, -0.0833333333333333, 0,
  -0.0223290993692602, -0.0833333333333333, 0.0833333333333333, 0.0223290993692602, 0,
  -0.0446581987385204, -0.166666666666667, -0.622008467928146, -0.166666666666667, 1 };

} // anonymous namespace

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkIntegrationGaussianStrategy);

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkIntegrationStrategy::PrintSelf(os, indent);
  os << indent << "Integration Strategy: Gaussian" << std::endl;
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegratePolyLine(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
  double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  this->LinearStrategy->IntegratePolyLine(
    input, output, cellId, numPts, cellPtIds, sum, sumCenter, cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegratePolygon(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
  double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  vtkWarningMacro("Cell type (" << input->GetCellType(cellId)
                                << ") is not yet supported by the Gaussian Integration Strategy. "
                                << "Computation falls back to Linear Strategy.");
  this->LinearStrategy->IntegratePolygon(
    input, output, cellId, numPts, cellPtIds, sum, sumCenter, cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateTriangleStrip(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
  double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  this->LinearStrategy->IntegrateTriangleStrip(
    input, output, cellId, numPts, cellPtIds, sum, sumCenter, cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateTriangle(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id,
  double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  this->LinearStrategy->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id, sum,
    sumCenter, cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateQuad(vtkDataSet* input, vtkUnstructuredGrid* output,
  vtkIdType cellId, vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id, double& sum,
  double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  std::vector<std::array<double, 3>> pts(4);
  double mid[3];
  input->GetPoint(pt1Id, pts[0].data());
  input->GetPoint(pt2Id, pts[1].data());
  input->GetPoint(pt3Id, pts[2].data());
  input->GetPoint(pt4Id, pts[3].data());

  vtkQuadratureSchemeDefinition* def = this->CellDefinitionDictionnary[VTK_QUAD];
  const double* QuadratureWeights = def->GetQuadratureWeights();
  int nQuadraturePoints = def->GetNumberOfQuadraturePoints();

  double area = 0.0;
  std::vector<double> partialArea(4);
  for (int qPtId = 0; qPtId < nQuadraturePoints; ++qPtId)
  {
    const double* dN = def->GetShapeFunctionDerivativeWeights(qPtId);
    double detJ = this->ComputeJacobianDet2D(dN, pts);
    partialArea[qPtId] = detJ * QuadratureWeights[qPtId];
    area += partialArea[qPtId];
  }

  sum += area;

  mid[0] = (pts[0][0] + pts[1][0] + pts[2][0] + pts[3][0]) / 4.0;
  mid[1] = (pts[0][1] + pts[1][1] + pts[2][1] + pts[3][1]) / 4.0;
  mid[2] = (pts[0][2] + pts[1][2] + pts[2][2] + pts[3][2]) / 4.0;
  sumCenter[0] += mid[0] * area;
  sumCenter[1] += mid[1] * area;
  sumCenter[2] += mid[2] * area;

  this->IntegratePointDataGaussian(
    output->GetPointData(), cellId, nQuadraturePoints, partialArea, pointFieldList, index);
  this->IntegrateData1(
    input->GetCellData(), output->GetCellData(), cellId, area, cellFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateTetrahedron(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id,
  vtkIdType pt4Id, double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  this->LinearStrategy->IntegrateTetrahedron(input, output, cellId, pt1Id, pt2Id, pt3Id, pt4Id, sum,
    sumCenter, cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegratePixel(vtkDataSet* input, vtkUnstructuredGrid* output,
  vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
  vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
  int index)
{
  this->LinearStrategy->IntegratePixel(
    input, output, cellId, numPts, cellPtIds, sum, sumCenter, cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateVoxel(vtkDataSet* input, vtkUnstructuredGrid* output,
  vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
  vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
  int index)
{
  this->LinearStrategy->IntegrateVoxel(
    input, output, cellId, numPts, cellPtIds, sum, sumCenter, cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateHexahedron(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkGenericCell* vtkNotUsed(cell), vtkIdType cellId, vtkIdType numPts,
  const vtkIdType* cellPtIds, vtkIdList* cellPtIdsList, double& sum, double sumCenter[3],
  vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
  int index)
{
  this->IntegrateGaussian(input, output, VTK_HEXAHEDRON, cellId, numPts, cellPtIds, cellPtIdsList,
    sum, sumCenter, cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateWedge(vtkDataSet* input, vtkUnstructuredGrid* output,
  vtkGenericCell* vtkNotUsed(cell), vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
  vtkIdList* cellPtIdsList, double& sum, double sumCenter[3],
  vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
  int index)
{
  this->IntegrateGaussian(input, output, VTK_WEDGE, cellId, numPts, cellPtIds, cellPtIdsList, sum,
    sumCenter, cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegratePyramid(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkGenericCell* vtkNotUsed(cell), vtkIdType cellId, vtkIdType numPts,
  const vtkIdType* cellPtIds, vtkIdList* cellPtIdsList, double& sum, double sumCenter[3],
  vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
  int index)
{
  this->IntegrateGaussian(input, output, VTK_PYRAMID, cellId, numPts, cellPtIds, cellPtIdsList, sum,
    sumCenter, cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateGeneral1DCell(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
  double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  this->LinearStrategy->IntegrateGeneral1DCell(
    input, output, cellId, numPts, cellPtIds, sum, sumCenter, cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateGeneral2DCell(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
  double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  this->LinearStrategy->IntegrateGeneral2DCell(
    input, output, cellId, numPts, cellPtIds, sum, sumCenter, cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateGeneral3DCell(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
  double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  this->LinearStrategy->IntegrateGeneral3DCell(
    input, output, cellId, numPts, cellPtIds, sum, sumCenter, cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateDefault(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkGenericCell* cell, vtkIdType cellId, vtkIdType numPts,
  vtkIdList* cellPtIds, double& sum, double sumCenter[3],
  vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
  int index)
{
  vtkWarningMacro("Cell type (" << input->GetCellType(cellId)
                                << ") is not yet supported by the Gaussian Integration Strategy. "
                                << "Computation falls back to Linear Strategy.");
  this->LinearStrategy->IntegrateDefault(input, output, cell, cellId, numPts, cellPtIds, sum,
    sumCenter, cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateData1(vtkDataSetAttributes* inda,
  vtkDataSetAttributes* outda, vtkIdType pt1Id, double k,
  vtkIntegrateAttributesFieldList& fieldlist, int fieldlist_index)
{
  this->LinearStrategy->IntegrateData1(inda, outda, pt1Id, k, fieldlist, fieldlist_index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateData2(vtkDataSetAttributes* inda,
  vtkDataSetAttributes* outda, vtkIdType pt1Id, vtkIdType pt2Id, double k,
  vtkIntegrateAttributesFieldList& fieldlist, int fieldlist_index)
{
  this->LinearStrategy->IntegrateData2(inda, outda, pt1Id, pt2Id, k, fieldlist, fieldlist_index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateData3(vtkDataSetAttributes* inda,
  vtkDataSetAttributes* outda, vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, double k,
  vtkIntegrateAttributesFieldList& fieldlist, int fieldlist_index)
{
  this->LinearStrategy->IntegrateData3(
    inda, outda, pt1Id, pt2Id, pt3Id, k, fieldlist, fieldlist_index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateData4(vtkDataSetAttributes* inda,
  vtkDataSetAttributes* outda, vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id,
  double k, vtkIntegrateAttributesFieldList& fieldlist, int fieldlist_index)
{
  this->LinearStrategy->IntegrateData4(
    inda, outda, pt1Id, pt2Id, pt3Id, pt4Id, k, fieldlist, fieldlist_index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegrateGaussian(vtkDataSet* input,
  vtkUnstructuredGrid* output, VTKCellType celltype, vtkIdType cellId, vtkIdType numPts,
  const vtkIdType* cellPtIds, vtkIdList* vtkNotUsed(cellPtIdsList), double& sum,
  double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  double mid[3] = { 0.0, 0.0, 0.0 };
  std::vector<std::array<double, 3>> pts(numPts);
  for (int i = 0; i < numPts; ++i)
  {
    input->GetPoint(cellPtIds[i], pts[i].data());
  }

  vtkQuadratureSchemeDefinition* def = this->CellDefinitionDictionnary[celltype];
  const double* QuadratureWeights = def->GetQuadratureWeights();
  int nQuadraturePoints = def->GetNumberOfQuadraturePoints();
  vtkIdType nNodes = def->GetNumberOfNodes();

  double volume = 0.0;
  std::vector<double> partialVolume(nQuadraturePoints);
  for (int qPtId = 0; qPtId < nQuadraturePoints; ++qPtId)
  {
    const double* dN = def->GetShapeFunctionDerivativeWeights(qPtId);
    double detJ = this->ComputeJacobianDet(dN, pts, nNodes);
    partialVolume[qPtId] = detJ * QuadratureWeights[qPtId];
    volume += partialVolume[qPtId];
  }

  sum += volume;

  for (int i = 0; i < numPts; ++i)
  {
    mid[0] += pts[i][0];
    mid[1] += pts[i][1];
    mid[2] += pts[i][2];
  }
  sumCenter[0] += mid[0] * volume / numPts;
  sumCenter[1] += mid[1] * volume / numPts;
  sumCenter[2] += mid[2] * volume / numPts;

  this->IntegratePointDataGaussian(
    output->GetPointData(), cellId, nQuadraturePoints, partialVolume, pointFieldList, index);
  this->IntegrateData1(
    input->GetCellData(), output->GetCellData(), cellId, volume, cellFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::IntegratePointDataGaussian(vtkDataSetAttributes* outda,
  vtkIdType cellId, int nQuadraturePoints, std::vector<double> partialVolume,
  vtkIntegrateAttributesFieldList& fieldList, int index)
{
  vtkIdType offsetIndex = this->Offsets->GetTuple1(cellId);

  auto f = [offsetIndex, partialVolume, nQuadraturePoints](
             vtkAbstractArray* ainArray, vtkAbstractArray* aoutArray)
  {
    vtkDataArray* inArray = vtkDataArray::FastDownCast(ainArray);
    vtkDoubleArray* outArray = vtkDoubleArray::FastDownCast(aoutArray);
    if (inArray && outArray)
    {
      const int numComponents = inArray->GetNumberOfComponents();
      for (int j = 0; j < numComponents; ++j)
      {
        double integral = 0.0;
        for (int i = 0; i < nQuadraturePoints; ++i)
        {
          integral += inArray->GetComponent(offsetIndex + i, j) * partialVolume[i];
        }
        const double vOut = integral + outArray->GetTypedComponent(0, j);
        outArray->SetTypedComponent(0, j, vOut);
      }
    }
  };

  fieldList.TransformData(index, this->Intermediate->GetFieldData(), outda, f);
}

//------------------------------------------------------------------------------
double vtkIntegrationGaussianStrategy::ComputeJacobianDet2D(
  const double dN[8], const std::vector<std::array<double, 3>>& pts)
{
  double JXi[3] = { 0.0, 0.0, 0.0 };
  double JEta[3] = { 0.0, 0.0, 0.0 };

  for (int i = 0; i < 4; ++i)
  {
    JXi[0] += dN[i] * pts[i][0];
    JXi[1] += dN[i] * pts[i][1];
    JXi[2] += dN[i] * pts[i][2];
    JEta[0] += dN[i + 4] * pts[i][0];
    JEta[1] += dN[i + 4] * pts[i][1];
    JEta[2] += dN[i + 4] * pts[i][2];
  }

  double cross[3];
  vtkMath::Cross(JXi, JEta, cross);
  return vtkMath::Norm(cross);
}

//------------------------------------------------------------------------------
double vtkIntegrationGaussianStrategy::ComputeJacobianDet(
  const double* dN, const std::vector<std::array<double, 3>>& pts, int nNodes)
{
  double J[3][3] = { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } };

  for (int i = 0; i < nNodes; i++)
  {
    J[0][0] += dN[i] * pts[i][0];
    J[0][1] += dN[i] * pts[i][1];
    J[0][2] += dN[i] * pts[i][2];

    J[1][0] += dN[i + nNodes] * pts[i][0];
    J[1][1] += dN[i + nNodes] * pts[i][1];
    J[1][2] += dN[i + nNodes] * pts[i][2];

    J[2][0] += dN[i + 2 * nNodes] * pts[i][0];
    J[2][1] += dN[i + 2 * nNodes] * pts[i][1];
    J[2][2] += dN[i + 2 * nNodes] * pts[i][2];
  }
  return vtkMath::Determinant3x3(J);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::Initialize(vtkDataSet* input)
{
  this->Intermediate = vtkSmartPointer<vtkDataSet>::NewInstance(input);
  this->Intermediate->ShallowCopy(input);

  this->AddPointDataArray(); // add at least 1 point data array, required by
                             // vtkQuadraturePointInterpolator
  this->InitializeQuadratureOffsets();
  this->ComputeQuadratureInterpolation();
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::AddPointDataArray()
{
  vtkSmartPointer<vtkDoubleArray> scalars = vtkSmartPointer<vtkDoubleArray>::New();
  scalars->SetName(this->GenerateUniqueArrayName("Volume").c_str());
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(this->Intermediate->GetNumberOfPoints());

  for (vtkIdType i = 0; i < this->Intermediate->GetNumberOfPoints(); i++)
  {
    scalars->SetValue(i, 1.0);
  }

  this->Intermediate->GetPointData()->AddArray(scalars);
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::InitializeQuadratureOffsets()
{
  vtkNew<vtkCellTypes> cellTypes;
  this->Intermediate->GetCellTypes(cellTypes);
  int nCellTypes = cellTypes ? cellTypes->GetNumberOfTypes() : 0;

  // Offsets = vtkSmartPointer<vtkIdTypeArray>::New();
  this->Offsets->SetName(this->GenerateUniqueArrayName("QuadratureOffset").c_str());
  this->Intermediate->GetCellData()->AddArray(this->Offsets);

  vtkInformation* info = this->Offsets->GetInformation();
  vtkInformationQuadratureSchemeDefinitionVectorKey* key =
    vtkQuadratureSchemeDefinition::DICTIONARY();

  for (int typeId = 0; typeId < nCellTypes; ++typeId)
  {
    int cellType = cellTypes->GetCellType(typeId);
    vtkSmartPointer<vtkQuadratureSchemeDefinition> def =
      this->CreateQuadratureSchemeDefinition(cellType);
    if (def)
    {
      key->Set(info, def, cellType);
    }
  }

  this->InitializeQuadratureOffsetsArray(info, key);
}

//------------------------------------------------------------------------------
std::string vtkIntegrationGaussianStrategy::GenerateUniqueArrayName(const std::string& baseName)
{
  std::string finalName = baseName;
  int i = 0;
  while (this->Intermediate->GetCellData()->GetArray(finalName.c_str()) != nullptr)
  {
    finalName = baseName + std::to_string(i++);
  }
  return finalName;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkQuadratureSchemeDefinition>
vtkIntegrationGaussianStrategy::CreateQuadratureSchemeDefinition(int cellType)
{
  vtkSmartPointer<vtkQuadratureSchemeDefinition> def =
    vtkSmartPointer<vtkQuadratureSchemeDefinition>::New();
  switch (cellType)
  {
    case VTK_QUAD:
      def->Initialize(VTK_QUAD, 4, 4, SFW_QUAD_BILINEAR, QW_QUAD_BILINEAR, 2, SFDW_QUAD_BILINEAR);
      break;
    case VTK_HEXAHEDRON:
      def->Initialize(
        VTK_HEXAHEDRON, 8, 8, SFW_HEX_TRILINEAR, QW_HEX_TRILINEAR, 3, SFDW_HEX_TRILINEAR);
      break;
    case VTK_WEDGE:
      def->Initialize(
        VTK_WEDGE, 6, 6, SFW_WEDGE_TRILINEAR, QW_WEDGE_TRILINEAR, 3, SFDW_WEDGE_TRILINEAR);
      break;
    case VTK_PYRAMID:
      def->Initialize(
        VTK_PYRAMID, 5, 8, SFW_PYR_TRILINEAR, QW_PYR_TRILINEAR, 3, SFDW_PYR_TRILINEAR);
      break;
    default:
      def->Initialize(VTK_EMPTY_CELL, 1, 1, nullptr, nullptr, 0, nullptr);
  }
  return def;
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::InitializeQuadratureOffsetsArray(
  vtkInformation* info, vtkInformationQuadratureSchemeDefinitionVectorKey* key)
{
  int dictSize = key->Size(info);
  this->CellDefinitionDictionnary = new vtkQuadratureSchemeDefinition*[dictSize];
  key->GetRange(info, this->CellDefinitionDictionnary, 0, 0, dictSize);

  this->Offsets->SetNumberOfTuples(this->Intermediate->GetNumberOfCells());
  vtkIdType offset = 0;
  for (vtkIdType cellid = 0; cellid < this->Intermediate->GetNumberOfCells(); cellid++)
  {
    this->Offsets->SetValue(cellid, offset);
    int cellType = this->Intermediate->GetCell(cellid)->GetCellType();
    vtkQuadratureSchemeDefinition* celldef = this->CellDefinitionDictionnary[cellType];
    offset += celldef->GetNumberOfQuadraturePoints();
  }
}

//------------------------------------------------------------------------------
void vtkIntegrationGaussianStrategy::ComputeQuadratureInterpolation()
{
  vtkNew<vtkQuadraturePointInterpolator> interpolator;
  interpolator->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, this->Offsets->GetName());
  interpolator->SetInputData(this->Intermediate);
  interpolator->Update();

  vtkDataSet* output = vtkDataSet::SafeDownCast(interpolator->GetOutput());
  this->Intermediate->ShallowCopy(output);
}

//------------------------------------------------------------------------------
vtkIntegrationGaussianStrategy::~vtkIntegrationGaussianStrategy()
{
  delete[] this->CellDefinitionDictionnary;
}

VTK_ABI_NAMESPACE_END
