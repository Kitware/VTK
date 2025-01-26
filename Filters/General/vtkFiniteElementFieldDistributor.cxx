// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFiniteElementFieldDistributor.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataAssembly.h"
#include "vtkDoubleArray.h"
#include "vtkHexahedron.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLagrangeCurve.h"
#include "vtkLagrangeHexahedron.h"
#include "vtkLagrangeQuadrilateral.h"
#include "vtkLagrangeTetra.h"
#include "vtkLagrangeTriangle.h"
#include "vtkLagrangeWedge.h"
#include "vtkLine.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkQuad.h"
#include "vtkStringArray.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkUnstructuredGrid.h"
#include "vtkWedge.h"

#include <array>
#include <cstddef>
#include <numeric>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{

const char* infoRecordName = "Information Records";

std::string GetEdgeCoefficientArrayName(const std::string& name)
{
  return std::string("EDGE_COEFF_") + name;
}

std::string GetFaceCoefficientArrayName(const std::string& name)
{
  return std::string("FACE_COEFF_") + name;
}

struct vtkFiniteElementSpec
{
  std::unordered_set<std::string> Fields;
  VTKCellType RefElement;
};

vtkDataArray* InitializeNewArray(
  vtkDataArray* in, const std::string& name, int ncomp, vtkIdType ntup)
{
  auto arr = in->NewInstance();
  arr->SetName(name.c_str());
  arr->SetNumberOfComponents(ncomp);
  arr->SetNumberOfTuples(ntup);
  arr->Fill(0.);
  return arr;
}

std::vector<std::string> Split(const std::string& inString, const std::string& delimiter)
{
  std::vector<std::string> subStrings;
  std::size_t sIdx = 0;
  std::size_t eIdx = 0;
  while ((eIdx = inString.find(delimiter, sIdx)) < inString.size())
  {
    subStrings.emplace_back(inString.substr(sIdx, eIdx - sIdx));
    sIdx = eIdx + delimiter.size();
  }
  if (sIdx < inString.size())
  {
    subStrings.emplace_back(inString.substr(sIdx));
  }
  return subStrings;
}

vtkPartitionedDataSet* GetNamedPartitionedDataSet(
  const std::string& name, vtkPartitionedDataSetCollection* input)
{
  vtkDataAssembly* assembly = input->GetDataAssembly();
  const std::string selector = "//" + vtkDataAssembly::MakeValidNodeName(name.c_str());
  std::vector<int> nodeIds = assembly->SelectNodes({ selector });

  if (nodeIds.empty())
  {
    return nullptr;
  }

  const auto ids = assembly->GetDataSetIndices(nodeIds[0]);
  if (ids.empty())
  {
    return nullptr;
  }
  return input->GetPartitionedDataSet(ids[0]);
}

std::vector<double> GetEdgeAttributes(const std::string& name, vtkCellData* cd, vtkIdType cellId)
{
  std::vector<double> attrs;
  vtkDataArray* coeffs = cd->GetArray(::GetEdgeCoefficientArrayName(name).c_str());
  if (coeffs == nullptr)
  {
    return attrs;
  }
  const int nEdges = coeffs->GetNumberOfComponents();
  attrs.resize(nEdges);
  coeffs->GetTuple(cellId, attrs.data());
  return attrs;
}

std::vector<double> GetFaceAttributes(const std::string& name, vtkCellData* cd, vtkIdType cellId)
{
  std::vector<double> attrs;
  vtkDataArray* coeffs = cd->GetArray(::GetFaceCoefficientArrayName(name).c_str());
  if (coeffs == nullptr)
  {
    return attrs;
  }
  const int nFaces = coeffs->GetNumberOfComponents();
  attrs.resize(nFaces);
  coeffs->GetTuple(cellId, attrs.data());
  return attrs;
}

std::vector<int> GetIOSSTransformation(VTKCellType cellType, int npts)
{
  std::vector<int> result;
  switch (cellType)
  {
    case VTK_LINE:
    case VTK_LAGRANGE_CURVE:
      switch (npts)
      {
        case 2:
        case 3:
        case 4:
          result.resize(npts, 0);
          std::iota(result.begin(), result.end(), 1);
          break;
        default:
          vtkLog(WARNING, << "Unsupported number of points for cell - VTK_LINE."
                          << "Supported: One of 2, 3, 4 "
                          << "Got: " << npts);
          break;
      }
      break;
    case VTK_TRIANGLE:
    case VTK_LAGRANGE_TRIANGLE:
      switch (npts)
      {
        case 3:
        case 6:
        case 10:
          result.resize(npts, 0);
          std::iota(result.begin(), result.end(), 1);
          break;
        default:
          vtkLog(WARNING, << "Unsupported number of points for cell - VTK_TRIANGLE."
                          << "Supported: One of 3, 6, 10"
                          << "Got: " << npts);
          break;
      }
      break;
    case VTK_QUAD:
    case VTK_LAGRANGE_QUADRILATERAL:
      switch (npts)
      {
        case 4:
        case 9:
        case 16:
          result.resize(npts, 0);
          std::iota(result.begin(), result.end(), 1);
          break;
        default:
          vtkLog(WARNING, << "Unsupported number of points for cell - VTK_QUAD."
                          << "Supported: One of 4, 9, 16 "
                          << "Got: " << npts);
          break;
      }
      break;
    case VTK_TETRA:
    case VTK_LAGRANGE_TETRAHEDRON:
      switch (npts)
      {
        case 4:
        case 10:
        case 11:
        case 15:
          result.resize(npts, 0);
          std::iota(result.begin(), result.end(), 1);
          break;
        default:
          vtkLog(WARNING, << "Unsupported number of points for cell - VTK_TETRA."
                          << "Supported: One of 4, 10, 11, 15 "
                          << "Got: " << npts);
          break;
      }
      break;
    case VTK_PYRAMID:
    case VTK_LAGRANGE_PYRAMID:
      // vtk does not have vtkHigherOrderPyramid.
      switch (npts)
      {
        case 5:
        case 13:
        case 14:
        case 19:
        default:
          vtkLog(WARNING, << "Unsupported number of points for cell - VTK_PYRAMID."
                          << "Supported: None "
                          << "Got: " << npts);
          break;
      }
      break;
    case VTK_WEDGE:
    case VTK_LAGRANGE_WEDGE:
      switch (npts)
      {
        case 6:
          result = { 4, 5, 6, 1, 2, 3 };
          break;
        case 15:
          // clang-format off
          result = {
            4, 5, 6, 1, 2, 3,
            13, 14, 15,
            7, 8, 9,
            10, 11, 12
          };
          // clang-format on
          break;
        case 18:
          // clang-format off
          result = {
            /* 2 triangles */
            4, 5, 6, 1, 2, 3,

            /* edge centers */
            13, 14, 15,
            7, 8, 9,
            10, 11, 12,

            /* quad-centers */
            16, 17, 18
          };
          // clang-format on
          break;
        case 21:
        {
          // XXX(gcc-12): GCC 12 has some weird warning triggered here where
          // `result.resize()` warns about writing out-of-bounds. Use a new
          // vector and move into `result` to avoid the warning.
          std::vector<int> r(npts);
          std::iota(r.begin(), r.end(), 1);
          result = std::move(r);
        }
        break;
        default:
          vtkLog(WARNING, << "Unsupported number of points for cell - VTK_WEDGE."
                          << "Supported: 15, 18, 21 "
                          << "Got: " << npts);
          break;
      }
      break;
    case VTK_HEXAHEDRON:
    case VTK_LAGRANGE_HEXAHEDRON:
      switch (npts)
      {
        case 8:
        {
          // XXX(gcc-12): GCC 12 has some weird warning triggered here where
          // `result.resize()` warns about writing out-of-bounds. Use a new
          // vector and move into `result` to avoid the warning.
          std::vector<int> r(npts);
          std::iota(r.begin(), r.end(), 1);
          result = std::move(r);
        }
        break;
        case 20:
          // clang-format off
          result = {
            /* 8 corners */
            1, 2, 3, 4,
            5, 6, 7, 8,

            /* 12 mid-edge nodes */
            9, 10, 11, 12,
            17, 18, 19, 20,
            13, 14, 15, 16
          };
          // clang-format on
          break;
        case 27:
          // clang-format off
          result = {
            /* 8 corners */
            1, 2, 3, 4,
            5, 6, 7, 8,

            /* 12 mid-edge nodes */
            9, 10, 11, 12,
            17, 18, 19, 20,
            13, 14, 15, 16,

            /* 6 mid-face nodes */
            24, 25, 26, 27, 22, 23,

            /* mid-volume node*/
            21
          };
          // clang-format on
          break;
        default:
          vtkLog(WARNING, << "Unsupported number of points for cell - VTK_HEXAHEDRON."
                          << "Supported: 8, 20, 27 "
                          << "Got: " << npts);
          break;
      }
      break;
    default:
      break;
  }
  return result;
}

/*
 * Vector Basis Lagrange Product Matrices
 * Generated from
 * [intrepid2](https://github.com/trilinos/Trilinos/tree/master/packages/intrepid2/src/Discretization/Basis)
 *
 */
using VblpMatrixType = std::vector<std::vector<std::array<double, 3>>>;
using VbfuncType = std::function<std::vector<std::array<double, 3>>(const double[3])>;
// the jacobian matrix is evaluated at each lagrange point. for n pts => n jacobians.
using JacobianMatricesType = std::vector<std::vector<std::vector<double>>>;
using JacobianFunctionsType = std::function<std::vector<std::vector<double>>(
  const double[3], const std::vector<double>& physCoords, std::size_t stride)>;

class vtkVectorBasisLagrangeProducts
{
public:
  vtkVectorBasisLagrangeProducts()
  {
    for (const auto& cellType : { VTK_HEXAHEDRON, VTK_QUAD, VTK_TETRA, VTK_TRIANGLE, VTK_WEDGE })
    {
      switch (cellType)
      {
        case VTK_HEXAHEDRON:
        {
          auto& hCurlPhi = this->HexVbf[0];
          // clang-format off
          hCurlPhi = [](const double p[3]) -> std::vector<std::array<double, 3>> {
            const double& x = p[0];
            const double& y = p[1];
            const double& z = p[2];
            return std::vector<std::array<double, 3>>({
              { (1.0 - y)*(1.0 - z)/4.0,   0.0,                      0.0                    },
              { 0.0,                       (1.0 + x)*(1.0 - z)/4.0,  0.0                    },
              { -(1.0 + y)*(1.0 - z)/4.0,  0.0,                      0.0                    },
              { 0.0,                       -(1.0 - x)*(1.0 - z)/4.0, 0.0                    },
              { (1.0 - y)*(1.0 + z)/4.0,   0.0,                      0.0                    },
              { 0.0,                       (1.0 + x)*(1.0 + z)/4.0,  0.0                    },
              { -(1.0 + y)*(1.0 + z)/4.0,  0.0,                      0.0                    },
              { 0.0,                       -(1.0 - x)*(1.0 + z)/4.0, 0.0                    },
              { 0.0,                       0.0,                      (1.0 - x)*(1.0 - y)/4.0 },
              { 0.0,                       0.0,                      (1.0 + x)*(1.0 - y)/4.0 },
              { 0.0,                       0.0,                      (1.0 + x)*(1.0 + y)/4.0 },
              { 0.0,                       0.0,                      (1.0 - x)*(1.0 + y)/4.0 }
            });
          };
          // clang-format on
          auto& hDivPhi = this->HexVbf[1];
          // clang-format off
          hDivPhi = [](const double p[3]) -> std::vector<std::array<double, 3>> {
            const double& x = p[0];
            const double& y = p[1];
            const double& z = p[2];
            return std::vector<std::array<double, 3>>({
              { 0.0,           (y - 1.0)/2.0, 0.0           },
              { (1.0 + x)/2.0, 0.0,           0.0           },
              { 0.0,           (1.0 + y)/2.0, 0.0           },
              { (x - 1.0)/2.0, 0.0,           0.0           },
              { 0.0,           0.0,           (z - 1.0)/2.0 },
              { 0.0,           0.0,           (1.0 + z)/2.0 }
            });
          };
          // clang-format on
          auto& jacFunc = this->HexJacF;
          // clang-format off
          jacFunc = [](const double refCoord[3], const std::vector<double>& physCoords, const std::size_t& stride) -> std::vector<std::vector<double>> {
            const double r = refCoord[0]; //NOLINT(readability-identifier-length)
            const double s = refCoord[1]; //NOLINT(readability-identifier-length)
            const double t = refCoord[2]; //NOLINT(readability-identifier-length)
            const std::size_t& npts = physCoords.size() / stride;
            assert(npts >= 8); // at least 8 nodes in a hex. quadratic hex can have more, but we don't use them.
            (void) npts;
            int xOfst = 0, yOfst = 1, zOfst = 2;
            // derivatives from https://github.com/trilinos/Trilinos/blob/master/packages/intrepid2/src/Discretization/Basis/Intrepid2_HGRAD_HEX_C1_FEMDef.hpp#L83
            return std::vector<std::vector<double>>({
              { // dx/dr
                (-(1.0 - s)*(1.0 - t)/8.0) * physCoords[xOfst]
              + (( 1.0 - s)*(1.0 - t)/8.0) * physCoords[xOfst + stride]
              + (( 1.0 + s)*(1.0 - t)/8.0) * physCoords[xOfst + 2 * stride]
              + (-(1.0 + s)*(1.0 - t)/8.0) * physCoords[xOfst + 3 * stride]
              + (-(1.0 - s)*(1.0 + t)/8.0) * physCoords[xOfst + 4 * stride]
              + ( (1.0 - s)*(1.0 + t)/8.0) * physCoords[xOfst + 5 * stride]
              + ( (1.0 + s)*(1.0 + t)/8.0) * physCoords[xOfst + 6 * stride]
              + (-(1.0 + s)*(1.0 + t)/8.0) * physCoords[xOfst + 7 * stride]
              , // dx/ds
                (-(1.0 - r)*(1.0 - t)/8.0) * physCoords[xOfst]
              + (-(1.0 + r)*(1.0 - t)/8.0) * physCoords[xOfst + stride]
              + ( (1.0 + r)*(1.0 - t)/8.0) * physCoords[xOfst + 2 * stride]
              + ( (1.0 - r)*(1.0 - t)/8.0) * physCoords[xOfst + 3 * stride]
              + (-(1.0 - r)*(1.0 + t)/8.0) * physCoords[xOfst + 4 * stride]
              + (-(1.0 + r)*(1.0 + t)/8.0) * physCoords[xOfst + 5 * stride]
              + ( (1.0 + r)*(1.0 + t)/8.0) * physCoords[xOfst + 6 * stride]
              + ( (1.0 - r)*(1.0 + t)/8.0) * physCoords[xOfst + 7 * stride]
              , // dx/dt
                (-(1.0 - r)*(1.0 - s)/8.0) * physCoords[xOfst]
              + (-(1.0 + r)*(1.0 - s)/8.0) * physCoords[xOfst + stride]
              + (-(1.0 + r)*(1.0 + s)/8.0) * physCoords[xOfst + 2 * stride]
              + (-(1.0 - r)*(1.0 + s)/8.0) * physCoords[xOfst + 3 * stride]
              + ( (1.0 - r)*(1.0 - s)/8.0) * physCoords[xOfst + 4 * stride]
              + ( (1.0 + r)*(1.0 - s)/8.0) * physCoords[xOfst + 5 * stride]
              + ( (1.0 + r)*(1.0 + s)/8.0) * physCoords[xOfst + 6 * stride]
              + ( (1.0 - r)*(1.0 + s)/8.0) * physCoords[xOfst + 7 * stride]
              },
              { // dy/dr
                (-(1.0 - s)*(1.0 - t)/8.0) * physCoords[yOfst]
              + (( 1.0 - s)*(1.0 - t)/8.0) * physCoords[yOfst + stride]
              + (( 1.0 + s)*(1.0 - t)/8.0) * physCoords[yOfst + 2 * stride]
              + (-(1.0 + s)*(1.0 - t)/8.0) * physCoords[yOfst + 3 * stride]
              + (-(1.0 - s)*(1.0 + t)/8.0) * physCoords[yOfst + 4 * stride]
              + ( (1.0 - s)*(1.0 + t)/8.0) * physCoords[yOfst + 5 * stride]
              + ( (1.0 + s)*(1.0 + t)/8.0) * physCoords[yOfst + 6 * stride]
              + (-(1.0 + s)*(1.0 + t)/8.0) * physCoords[yOfst + 7 * stride]
              , // dy/ds
                (-(1.0 - r)*(1.0 - t)/8.0) * physCoords[yOfst]
              + (-(1.0 + r)*(1.0 - t)/8.0) * physCoords[yOfst + stride]
              + ( (1.0 + r)*(1.0 - t)/8.0) * physCoords[yOfst + 2 * stride]
              + ( (1.0 - r)*(1.0 - t)/8.0) * physCoords[yOfst + 3 * stride]
              + (-(1.0 - r)*(1.0 + t)/8.0) * physCoords[yOfst + 4 * stride]
              + (-(1.0 + r)*(1.0 + t)/8.0) * physCoords[yOfst + 5 * stride]
              + ( (1.0 + r)*(1.0 + t)/8.0) * physCoords[yOfst + 6 * stride]
              + ( (1.0 - r)*(1.0 + t)/8.0) * physCoords[yOfst + 7 * stride]
              , // dy/dt
                (-(1.0 - r)*(1.0 - s)/8.0) * physCoords[yOfst]
              + (-(1.0 + r)*(1.0 - s)/8.0) * physCoords[yOfst + stride]
              + (-(1.0 + r)*(1.0 + s)/8.0) * physCoords[yOfst + 2 * stride]
              + (-(1.0 - r)*(1.0 + s)/8.0) * physCoords[yOfst + 3 * stride]
              + ( (1.0 - r)*(1.0 - s)/8.0) * physCoords[yOfst + 4 * stride]
              + ( (1.0 + r)*(1.0 - s)/8.0) * physCoords[yOfst + 5 * stride]
              + ( (1.0 + r)*(1.0 + s)/8.0) * physCoords[yOfst + 6 * stride]
              + ( (1.0 - r)*(1.0 + s)/8.0) * physCoords[yOfst + 7 * stride]
              },
              { // dz/dr
                (-(1.0 - s)*(1.0 - t)/8.0) * physCoords[zOfst]
              + (( 1.0 - s)*(1.0 - t)/8.0) * physCoords[zOfst + stride]
              + (( 1.0 + s)*(1.0 - t)/8.0) * physCoords[zOfst + 2 * stride]
              + (-(1.0 + s)*(1.0 - t)/8.0) * physCoords[zOfst + 3 * stride]
              + (-(1.0 - s)*(1.0 + t)/8.0) * physCoords[zOfst + 4 * stride]
              + ( (1.0 - s)*(1.0 + t)/8.0) * physCoords[zOfst + 5 * stride]
              + ( (1.0 + s)*(1.0 + t)/8.0) * physCoords[zOfst + 6 * stride]
              + (-(1.0 + s)*(1.0 + t)/8.0) * physCoords[zOfst + 7 * stride]
              , // dz/ds
                (-(1.0 - r)*(1.0 - t)/8.0) * physCoords[zOfst]
              + (-(1.0 + r)*(1.0 - t)/8.0) * physCoords[zOfst + stride]
              + ( (1.0 + r)*(1.0 - t)/8.0) * physCoords[zOfst + 2 * stride]
              + ( (1.0 - r)*(1.0 - t)/8.0) * physCoords[zOfst + 3 * stride]
              + (-(1.0 - r)*(1.0 + t)/8.0) * physCoords[zOfst + 4 * stride]
              + (-(1.0 + r)*(1.0 + t)/8.0) * physCoords[zOfst + 5 * stride]
              + ( (1.0 + r)*(1.0 + t)/8.0) * physCoords[zOfst + 6 * stride]
              + ( (1.0 - r)*(1.0 + t)/8.0) * physCoords[zOfst + 7 * stride]
              , // dz/dt
                (-(1.0 - r)*(1.0 - s)/8.0) * physCoords[zOfst]
              + (-(1.0 + r)*(1.0 - s)/8.0) * physCoords[zOfst + stride]
              + (-(1.0 + r)*(1.0 + s)/8.0) * physCoords[zOfst + 2 * stride]
              + (-(1.0 - r)*(1.0 + s)/8.0) * physCoords[zOfst + 3 * stride]
              + ( (1.0 - r)*(1.0 - s)/8.0) * physCoords[zOfst + 4 * stride]
              + ( (1.0 + r)*(1.0 - s)/8.0) * physCoords[zOfst + 5 * stride]
              + ( (1.0 + r)*(1.0 + s)/8.0) * physCoords[zOfst + 6 * stride]
              + ( (1.0 - r)*(1.0 + s)/8.0) * physCoords[zOfst + 7 * stride]
              }
            });
          };
          // clang-format on
          break;
        }
        case VTK_QUAD:
        {
          auto& hCurlPhi = this->QuadVbf[0];
          // clang-format off
          hCurlPhi = [](const double p[3]) -> std::vector<std::array<double, 3>> {
            const double& x = p[0];
            const double& y = p[1];
            return std::vector<std::array<double, 3>>({
              { 0.5 * (1. - y),  0.0,             0.0 },
              { 0.0,             0.5 * (1. + x),  0.0 },
              { -0.5 * (1. + y), 0.0,             0.0 },
              { 0.0,             -0.5 * (1. - x), 0.0 }
            });
          };
          // clang-format on
          auto& hDivPhi = this->QuadVbf[1];
          // clang-format off
          hDivPhi = [](const double p[3]) -> std::vector<std::array<double, 3>> {
            const double& x = p[0];
            const double& y = p[1];
            return std::vector<std::array<double, 3>>({
              { 0.0,           0.5*(y - 1.0), 0.0 },
              { 0.5*(1.0 + x), 0.0,           0.0 },
              { 0.0,           0.5*(1.0 + y), 0.0 },
              { 0.5*(x - 1.0), 0.0,           0.0 }
            });
          };
          // clang-format on
          auto& jacFunc = this->QuadJacF;
          // clang-format on;
          // clang-format off
          jacFunc = [](const double refCoord[3], const std::vector<double>& physCoords, const std::size_t& stride) -> std::vector<std::vector<double>> {
            const double r = refCoord[0]; //NOLINT(readability-identifier-length)
            const double s = refCoord[1]; //NOLINT(readability-identifier-length)
            const std::size_t& npts = physCoords.size() / stride;
            assert(npts >= 3); // at least 3 nodes in a triangle. quadratic triangle can have more, but we don't use them.
            (void) npts;
            int xOfst = 0, yOfst = 1;
            // derivatives from https://github.com/trilinos/Trilinos/blob/master/packages/intrepid2/src/Discretization/Basis/Intrepid2_HGRAD_QUAD_C1_FEMDef.hpp#L78
            return std::vector<std::vector<double>>({
              { // dx/dr
                (-(1.0 - s) / 4.0) * physCoords[xOfst]
              + ( (1.0 - s) / 4.0) * physCoords[xOfst + stride]
              + ( (1.0 + s) / 4.0) * physCoords[xOfst + 2 * stride]
              + (-(1.0 + s) / 4.0) * physCoords[xOfst + 3 * stride]
              , // dx/ds
                (-(1.0 - r) / 4.0) * physCoords[xOfst]
              + (-(1.0 + r) / 4.0) * physCoords[xOfst + stride]
              + ( (1.0 + r) / 4.0) * physCoords[xOfst + 2 * stride]
              + ( (1.0 - r) / 4.0) * physCoords[xOfst + 3 * stride]
              , // dx/dt
                0
              },
              { // dy/dr
                (-(1.0 - s) / 4.0) * physCoords[yOfst]
              + ( (1.0 - s) / 4.0) * physCoords[yOfst + stride]
              + ( (1.0 + s) / 4.0) * physCoords[yOfst + 2 * stride]
              + (-(1.0 + s) / 4.0) * physCoords[yOfst + 3 * stride]
              , // dy/ds
                (-(1.0 - r) / 4.0) * physCoords[yOfst]
              + (-(1.0 + r) / 4.0) * physCoords[yOfst + stride]
              + ( (1.0 + r) / 4.0) * physCoords[yOfst + 2 * stride]
              + ( (1.0 - r) / 4.0) * physCoords[yOfst + 3 * stride]
              , // dy/dt
                0
              },
              { // dz/dr
                0
              , // dz/ds
                0
              , // dz/dt
                0
              },
            });
          };
          // clang-format on
          break;
        }
        case VTK_TETRA:
        {
          auto& hCurlPhi = this->TetVbf[0];
          // clang-format off
          hCurlPhi = [](const double p[3]) -> std::vector<std::array<double, 3>> {
            const double& x = p[0];
            const double& y = p[1];
            const double& z = p[2];
            return std::vector<std::array<double, 3>>({
              { 2.0 * (1.0 - y - z), 2.0 * x,              2.0 * x             },
              { -2.0 * y,            2.0 * x,              0.0                 },
              { -2.0 * y,            2.0 * (-1.0 + x + z), -2.0 * y            },
              { 2.0 * z,             2.0 * z,              2.0 * (1.0 - x - y) },
              { -2.0 * z,            0.0,                  2.0 * x             },
              { 0.0,                -2.0 * z,              2.0 * y             }
            });
          };
          // clang-format on
          auto& hDivPhi = this->TetVbf[1];
          // clang-format off
          hDivPhi = [](const double p[3]) -> std::vector<std::array<double, 3>>
          {
            const double& x = p[0];
            const double& y = p[1];
            const double& z = p[2];
            return std::vector<std::array<double, 3>>({
              { x,       y - 1.0, z },
              { x,       y,       z },
              { x - 1.0, y, z       },
              { x,       y, z - 1.0 }
           });
          };
          // clang-format on
          auto& jacFunc = this->TetJacF;
          // clang-format off
          jacFunc = [](const double vtkNotUsed(refCoord)[3], const std::vector<double>& physCoords, const std::size_t& stride) -> std::vector<std::vector<double>> {
            const std::size_t& npts = physCoords.size() / stride;
            assert(npts >= 4); // at least 4 nodes in a tet. quad tet can have more, but we don't use them.
            (void) npts;
            int xOfst = 0, yOfst = 1, zOfst = 2;
            // derivatives from https://github.com/trilinos/Trilinos/blob/master/packages/intrepid2/src/Discretization/Basis/Intrepid2_HGRAD_TET_C1_FEMDef.hpp#L79
            return std::vector<std::vector<double>>({
              { // dx/dr
                -1 * physCoords[xOfst]
              +  1 * physCoords[xOfst + stride]
              +  0
              +  0
              , // dx/ds
                -1 * physCoords[xOfst]
              +  0
              +  1 * physCoords[xOfst + 2 * stride]
              +  0
              , // dx/dt
                -1 * physCoords[xOfst]
              +  0
              +  0
              +  1 * physCoords[xOfst + 3 * stride]
              },
              { // dy/dr
                -1 * physCoords[yOfst]
              +  1 * physCoords[yOfst + stride]
              +  0
              +  0
              , // dy/ds
                -1 * physCoords[yOfst]
              +  0
              +  1 * physCoords[yOfst + 2 * stride]
              +  0
              , // dy/dt
                -1 * physCoords[yOfst]
              +  0
              +  0
              +  1 * physCoords[yOfst + 3 * stride]
              },
              { // dz/dr
                -1 * physCoords[zOfst]
              +  1 * physCoords[zOfst + stride]
              +  0
              +  0
              , // dz/ds
                -1 * physCoords[zOfst]
              +  0
              +  1 * physCoords[zOfst + 2 * stride]
              +  0
              , // dz/dt
                -1 * physCoords[zOfst]
              +  0
              +  0
              +  1 * physCoords[zOfst + 3 * stride]
              },
            });
          };
          // clang-format on
          break;
        }
        case VTK_TRIANGLE:
        {
          auto& hCurlPhi = this->TriVbf[0];
          // clang-format off
          hCurlPhi = [](const double p[3]) -> std::vector<std::array<double, 3>> {
            const double& x = p[0];
            const double& y = p[1];
            return std::vector<std::array<double, 3>>({
              { 2.0 * (1. - y), 2.0 * x,        0.0 },
              { -2.0 * y,       2.0 * x,        0.0 },
              { -2.0 * y,       2.0 * (-1. + x),0.0 }
            });
          };
          // clang-format on
          auto& hDivPhi = this->TriVbf[1];
          // clang-format off
          hDivPhi = [](const double p[3]) -> std::vector<std::array<double, 3>> {
            const double& x = p[0];
            const double& y = p[1];
            return std::vector<std::array<double, 3>>({
              { 2.0 * x,         2.0 * (y - 1.0), 0.0 },
              { 2.0 * x,         2.0 * y,         0.0 },
              { 2.0 * (x - 1.0), 2.0 * y,         0.0 }
            });
          };
          // clang-format on
          auto& jacFunc = this->TriJacF;
          // clang-format off
          jacFunc = [](const double vtkNotUsed(refCoord)[3], const std::vector<double>& physCoords, const std::size_t& stride) -> std::vector<std::vector<double>> {
            const std::size_t& npts = physCoords.size() / stride;
            assert(npts >= 3); // at least 3 nodes in a triangle. quadratic triangle can have more, but we don't use them.
            (void) npts;
            int xOfst = 0, yOfst = 1;
            // derivatives from https://github.com/trilinos/Trilinos/blob/master/packages/intrepid2/src/Discretization/Basis/Intrepid2_HGRAD_TRI_C1_FEMDef.hpp#L78
            return std::vector<std::vector<double>>({
              { // dx/dr
                -1 * physCoords[xOfst]
              +  1 * physCoords[xOfst + stride]
              +  0
              , // dx/ds
                -1 * physCoords[xOfst]
              +  0
              +  1 * physCoords[xOfst + 2 * stride]
              , // dx/dt
                0
              },
              { // dy/dr
                -1 * physCoords[yOfst]
              +  1 * physCoords[yOfst + stride]
              +  0
              , // dy/ds
                -1 * physCoords[yOfst]
              +  0
              +  1 * physCoords[yOfst + 2 * stride]
              , // dy/dt
                0
              },
              { // dz/dr
                0
              , // dz/ds
                0
              , // dz/dt
                0
              },
            });
          };
          // clang-format on
          break;
        }
        case VTK_WEDGE:
        {
          auto& hCurlPhi = this->WedgeVbf[0];
          // clang-format off
            hCurlPhi = [](const double p[3]) -> std::vector<std::array<double, 3>> {
              const double& x = p[0];
              const double& y = p[1];
              const double& z = p[2];
              return std::vector<std::array<double, 3>>({
                { (1.0 - z)*(1.0 - y), x*(1.0 - z),         0.0           },
                { y*(z - 1.0),         x*(1.0 - z),         0.0           },
                { y*(z - 1.0),         (1.0 - x)*(z - 1.0), 0.0           },
                { (1.0 - y)*(1.0 + z), x*(1.0 + z),         0.0           },
                { -y*(1.0 + z),        x*(1.0 + z),         0.0           },
                { -y*(1.0 + z),        (x - 1.0)*(1.0 + z), 0.0           },
                { 0.0,                 0.0,                 (1.0 - x - y) },
                { 0.0,                 0.0,                  x            },
                { 0.0,                 0.0,                  y            }
              });
            };
          // clang-format on
          auto& hDivPhi = this->WedgeVbf[1];
          // clang-format off
          hDivPhi = [](const double p[3]) -> std::vector<std::array<double, 3>> {
            const double& x = p[0];
            const double& y = p[1];
            const double& z = p[2];
            return std::vector<std::array<double, 3>>({
              { 2.0 * x,         2.0 * (y - 1.0), 0.0             },
              { 2.0 * x,         2.0 * y,         0.0             },
              { 2.0 * (x - 1.0), 2.0 * y,         0.0             },
              { 0.0,             0.0,             (z - 1.0) / 2.0 },
              { 0.0,             0.0,             (1.0 + z) / 2.0 }
            });
          };
          // clang-format on
          auto& jacFunc = this->WedgeJacF;
          // clang-format off
          jacFunc = [](const double refCoord[3], const std::vector<double>& physCoords, const std::size_t& stride) -> std::vector<std::vector<double>> {
            const double r = refCoord[0]; //NOLINT(readability-identifier-length)
            const double s = refCoord[1]; //NOLINT(readability-identifier-length)
            const double t = refCoord[2]; //NOLINT(readability-identifier-length)
            const std::size_t& npts = physCoords.size() / stride;
            assert(npts >= 6); // at least 6 nodes in a wedge. quadratic wedge can have more, but we don't use them.
            (void) npts;
            int xOfst = 0, yOfst = 1, zOfst = 2;
            // derivatives from https://github.com/trilinos/Trilinos/blob/master/packages/intrepid2/src/Discretization/Basis/Intrepid2_HGRAD_WEDGE_C1_FEMDef.hpp#L81
            return std::vector<std::vector<double>>({
              { // dx/dr
                (-(1.0 - t)/2.0) * physCoords[xOfst]
              + ( (1.0 - t)/2.0) * physCoords[xOfst + stride]
              + (-(1.0 + t)/2.0) * physCoords[xOfst + 3 * stride]
              + ( (1.0 + t)/2.0) * physCoords[xOfst + 4 * stride]
              , // dx/ds
                (-(1.0 - t)/2.0) * physCoords[xOfst]
              + ( (1.0 - t)/2.0) * physCoords[xOfst + 2 * stride]
              + (-(1.0 + t)/2.0) * physCoords[xOfst + 3 * stride]
              + ( (1.0 + t)/2.0) * physCoords[xOfst + 5 * stride]
              , // dx/dt
                (-(1.0 - r - s)/2.0) * physCoords[xOfst]
              + (-r/2.0) * physCoords[xOfst + stride]
              + (-s/2.0) * physCoords[xOfst + 2 * stride]
              + ( (1.0 - r - s)/2.0) * physCoords[xOfst + 3 * stride]
              + ( r/2.0) * physCoords[xOfst + 4 * stride]
              + ( s/2.0) * physCoords[xOfst + 5 * stride]
              },
              { // dy/dr
                (-(1.0 - t)/2.0) * physCoords[yOfst]
              + ( (1.0 - t)/2.0) * physCoords[yOfst + stride]
              + (-(1.0 + t)/2.0) * physCoords[yOfst + 3 * stride]
              + ( (1.0 + t)/2.0) * physCoords[yOfst + 4 * stride]
              , // dy/ds
                (-(1.0 - t)/2.0) * physCoords[yOfst]
              + ( (1.0 - t)/2.0) * physCoords[yOfst + 2 * stride]
              + (-(1.0 + t)/2.0) * physCoords[yOfst + 3 * stride]
              + ( (1.0 + t)/2.0) * physCoords[yOfst + 5 * stride]
              , // dy/dt
                (-(1.0 - r - s)/2.0) * physCoords[yOfst]
              + (-r/2.0) * physCoords[yOfst + stride]
              + (-s/2.0) * physCoords[yOfst + 2 * stride]
              + ( (1.0 - r - s)/2.0) * physCoords[yOfst + 3 * stride]
              + ( r/2.0) * physCoords[yOfst + 4 * stride]
              + ( s/2.0) * physCoords[yOfst + 5 * stride]
              },
              { // dz/dr
                (-(1.0 - t)/2.0) * physCoords[zOfst]
              + ( (1.0 - t)/2.0) * physCoords[zOfst + stride]
              + (-(1.0 + t)/2.0) * physCoords[zOfst + 3 * stride]
              + ( (1.0 + t)/2.0) * physCoords[zOfst + 4 * stride]
              , // dz/ds
                (-(1.0 - t)/2.0) * physCoords[zOfst]
              + ( (1.0 - t)/2.0) * physCoords[zOfst + 2 * stride]
              + (-(1.0 + t)/2.0) * physCoords[zOfst + 3 * stride]
              + ( (1.0 + t)/2.0) * physCoords[zOfst + 5 * stride]
              , // dz/dt
                (-(1.0 - r - s)/2.0) * physCoords[zOfst]
              + (-r/2.0) * physCoords[zOfst + stride]
              + (-s/2.0) * physCoords[zOfst + 2 * stride]
              + ( (1.0 - r - s)/2.0) * physCoords[zOfst + 3 * stride]
              + ( r/2.0) * physCoords[zOfst + 4 * stride]
              + ( s/2.0) * physCoords[zOfst + 5 * stride]
              }
            });
          };
          // clang-format on
          break;
        }
        default:
          break;
      }
    }
  }

  ~vtkVectorBasisLagrangeProducts() = default;

  enum class SpaceType : int
  {
    HCurl = 0,
    HDiv
  };

  void Initialize(VTKCellType cellType, const double* coords, size_t npts)
  {
    auto hCurlMats = this->GetVblp(SpaceType::HCurl, cellType);
    auto hDivMats = this->GetVblp(SpaceType::HDiv, cellType);

    const auto hCurlVbfs = this->GetVbFunctions(SpaceType::HCurl, cellType);
    const auto hDivVbfs = this->GetVbFunctions(SpaceType::HDiv, cellType);
    auto jacMats = this->GetJacobianMatrices(cellType);

    if (hCurlMats == nullptr || hDivMats == nullptr || hCurlVbfs == nullptr || hDivVbfs == nullptr)
    {
      vtkLog(ERROR, "Unsupported request for cellType: " << cellType);
      return;
    }

    hCurlMats->clear();
    hDivMats->clear();

    auto& hCurlVbFunc = (*hCurlVbfs);
    auto& hDivVbfunc = (*hDivVbfs);
    ptrdiff_t ofst = 0;
    for (size_t i = 0; i < npts; ++i, ++ofst)
    {
      double coord[3] = { coords[ofst], coords[++ofst], coords[++ofst] };
      hCurlMats->emplace_back(hCurlVbFunc(coord));
      hDivMats->emplace_back(hDivVbfunc(coord));
    }
    jacMats->resize(npts);
    // jacobian will always be 3 irrespective of cell dimensionality.
    // for 2d cells, keep an extra row and column with 0. inefficient, but makes code readable.
    for (size_t ptId = 0; ptId < npts; ++ptId)
    {
      auto& jac = (*jacMats)[ptId];
      jac.resize(3, std::vector<double>(3, 0));
    }
  }

  void ComputeJacobian(VTKCellType cellType, const double* refCoords, size_t npts,
    const vtkIdType* ptIds, vtkPoints* physPoints)
  {
    auto jacMats = this->GetJacobianMatrices(cellType);
    auto jacFuncs = this->GetJacobianFunctions(cellType);
    if (jacMats == nullptr || jacFuncs == nullptr)
    {
      vtkLog(ERROR, "Unsupported request for cellType: " << cellType);
      return;
    }
    assert(jacMats->size() == npts);

    std::vector<double> physCoords(npts * 3);
    for (size_t ptId = 0; ptId < npts; ++ptId)
    {
      physPoints->GetPoint(ptIds[ptId], &physCoords[ptId * 3]);
    }

    ptrdiff_t ofst = 0;
    for (size_t ptId = 0; ptId < npts; ++ptId, ++ofst)
    {
      double refCoord[3] = { refCoords[ofst], refCoords[++ofst], refCoords[++ofst] };

      auto& jacFunction = (*jacFuncs);
      auto& jacMatrix = (*jacMats)[ptId];

      jacMatrix = jacFunction(refCoord, physCoords, 3);
    }
  }

  bool RequiresInitialization(VTKCellType cellType, size_t npts)
  {
    auto hCurlMats = this->GetVblp(SpaceType::HCurl, cellType);
    auto hDivMats = this->GetVblp(SpaceType::HDiv, cellType);
    auto jacMats = this->GetJacobianMatrices(cellType);
    if (hCurlMats == nullptr || hDivMats == nullptr || jacMats == nullptr)
    {
      vtkLog(ERROR, "Unsupported request for cellType: " << cellType);
      return false;
    }

    return hCurlMats->size() != npts || hDivMats->size() != npts || jacMats->empty();
  }

  void Clear(VTKCellType cellType)
  {
    auto hCurlMats = this->GetVblp(SpaceType::HCurl, cellType);
    auto hDivMats = this->GetVblp(SpaceType::HDiv, cellType);
    auto jacMats = this->GetJacobianMatrices(cellType);
    if (hCurlMats == nullptr || hDivMats == nullptr || jacMats == nullptr)
    {
      vtkLog(ERROR, "Unsupported request for cellType: " << cellType);
      return;
    }
    hCurlMats->clear();
    hDivMats->clear();
    jacMats->clear();
  }

  ::VblpMatrixType* GetVblp(SpaceType space, VTKCellType cellType)
  {
    ::VblpMatrixType* mats = nullptr;
    switch (cellType)
    {
      case VTK_HEXAHEDRON:
        mats = &(this->HexVblpMats[static_cast<int>(space)]);
        break;
      case VTK_QUAD:
        mats = &(this->QuadVblpMats[static_cast<int>(space)]);
        break;
      case VTK_TETRA:
        mats = &(this->TetVblpMats[static_cast<int>(space)]);
        break;
      case VTK_TRIANGLE:
        mats = &(this->TriVblpMats[static_cast<int>(space)]);
        break;
      case VTK_WEDGE:
        mats = &(this->WedgeVblpMats[static_cast<int>(space)]);
        break;
      default:
        break;
    }
    return mats;
  }

  ::VbfuncType* GetVbFunctions(SpaceType space, VTKCellType cellType)
  {
    const int space_int = static_cast<int>(space);
    switch (cellType)
    {
      case VTK_HEXAHEDRON:
        return &this->HexVbf[space_int];
      case VTK_QUAD:
        return &this->QuadVbf[space_int];
      case VTK_TETRA:
        return &this->TetVbf[space_int];
      case VTK_TRIANGLE:
        return &this->TriVbf[space_int];
      case VTK_WEDGE:
        return &this->WedgeVbf[space_int];
      default:
        break;
    }
    return nullptr;
  }

  ::JacobianMatricesType* GetJacobianMatrices(VTKCellType cellType)
  {
    ::JacobianMatricesType* mats = nullptr;
    switch (cellType)
    {
      case VTK_HEXAHEDRON:
        mats = &(this->HexJacMats);
        break;
      case VTK_QUAD:
        mats = &(this->QuadJacMats);
        break;
      case VTK_TETRA:
        mats = &(this->TetJacMats);
        break;
      case VTK_TRIANGLE:
        mats = &(this->TriJacMats);
        break;
      case VTK_WEDGE:
        mats = &(this->WedgeJacMats);
        break;
      default:
        break;
    }
    return mats;
  }

  ::JacobianFunctionsType* GetJacobianFunctions(VTKCellType cellType)
  {
    switch (cellType)
    {
      case VTK_HEXAHEDRON:
        return &this->HexJacF;
      case VTK_QUAD:
        return &this->QuadJacF;
      case VTK_TETRA:
        return &this->TetJacF;
      case VTK_TRIANGLE:
        return &this->TriJacF;
      case VTK_WEDGE:
        return &this->WedgeJacF;
      default:
        break;
    }
    return nullptr;
  }

  ::VblpMatrixType ReferenceToPhysical(const ::VblpMatrixType& vblpIn,
    const ::JacobianMatricesType& jacobianMats,
    const vtkVectorBasisLagrangeProducts::SpaceType spaceType)
  {
    auto result = vblpIn;
    const auto numPoints = jacobianMats.size();
    assert(numPoints <= vblpIn.size());
    switch (spaceType)
    {
      case SpaceType::HCurl:
        for (std::size_t ptId = 0; ptId < numPoints; ++ptId)
        {
          const auto& jac = jacobianMats[ptId];
          const auto& vblp = vblpIn[ptId];
          const auto& numVectors = vblpIn[ptId].size();
          const double jacT[3][3] = {
            { jac[0][0], jac[1][0], jac[2][0] },
            { jac[0][1], jac[1][1], jac[2][1] },
            { jac[0][2], jac[1][2], jac[2][2] },
          };
          double piola[3][3] = {};
          if (jac[2][0] == 0.0 && jac[2][1] == 0.0 && jac[2][2] == 0.0)
          {
            vtkMath::InvertMatrix<2>(jacT, piola);
          }
          else
          {
            vtkMath::Invert3x3(jacT, piola);
          }
          for (std::size_t vectId = 0; vectId < numVectors; ++vectId)
          {
            const double& v_r = vblp[vectId][0];
            const double& v_s = vblp[vectId][1];
            const double& v_t = vblp[vectId][2];

            double xyz[3];
            xyz[0] = piola[0][0] * v_r + piola[0][1] * v_s + piola[0][2] * v_t;
            xyz[1] = piola[1][0] * v_r + piola[1][1] * v_s + piola[1][2] * v_t;
            xyz[2] = piola[2][0] * v_r + piola[2][1] * v_s + piola[2][2] * v_t;

            result[ptId][vectId][0] = xyz[0];
            result[ptId][vectId][1] = xyz[1];
            result[ptId][vectId][2] = xyz[2];
          }
        }
        break;
      case SpaceType::HDiv:
        for (std::size_t ptId = 0; ptId < numPoints; ++ptId)
        {
          const auto& jac = jacobianMats[ptId];
          const auto& vblp = vblpIn[ptId];
          const auto& numVectors = vblpIn[ptId].size();
          double det = 0.0;
          if (jac[2][0] == 0.0 && jac[2][1] == 0.0 && jac[2][2] == 0.0)
          {
            det = vtkMath::Determinant2x2(jac[0][0], jac[0][1], jac[1][0], jac[1][1]);
          }
          else
          {
            det = vtkMath::Determinant3x3(jac[0][0], jac[1][0], jac[2][0], jac[0][1], jac[1][1],
              jac[2][1], jac[0][2], jac[1][2], jac[2][2]);
          }
          for (std::size_t vectId = 0; vectId < numVectors; ++vectId)
          {
            const double& v_r = vblp[vectId][0];
            const double& v_s = vblp[vectId][1];
            const double& v_t = vblp[vectId][2];

            double xyz[3];
            xyz[0] = jac[0][0] * v_r + jac[0][1] * v_s + jac[0][2] * v_t;
            xyz[1] = jac[1][0] * v_r + jac[1][1] * v_s + jac[1][2] * v_t;
            xyz[2] = jac[2][0] * v_r + jac[2][1] * v_s + jac[2][2] * v_t;

            result[ptId][vectId][0] = xyz[0] / det;
            result[ptId][vectId][1] = xyz[1] / det;
            result[ptId][vectId][2] = xyz[2] / det;
          }
        }
        break;
      default:
        break;
    }
    return result;
  }

private:
  ::VblpMatrixType HexVblpMats[2], QuadVblpMats[2], TetVblpMats[2], TriVblpMats[2],
    WedgeVblpMats[2];
  ::VbfuncType HexVbf[2], QuadVbf[2], TetVbf[2], TriVbf[2], WedgeVbf[2];
  // jacobians are hCurl/hDiv agnostic. there's only one set of matrices per celltype these
  // are computed with lagrange basis functions.
  // Ex: the jacobian for hex at node 0 -> HexJacMats[0] = 3x3 matrix and so on for other nodes
  // 1,...7
  ::JacobianMatricesType HexJacMats, QuadJacMats, TetJacMats, TriJacMats, WedgeJacMats;
  ::JacobianFunctionsType HexJacF, QuadJacF, TetJacF, TriJacF, WedgeJacF;
}; // end vtkVectorBasisLagrangeProducts

using SpaceType = vtkVectorBasisLagrangeProducts::SpaceType;

void InterpolateToNodes(const ::VblpMatrixType& vblpmats, const std::vector<double>& coeffs,
  vtkIdType npts, const vtkIdType* pts, vtkDataArray* result)
{
  const std::size_t& nDofs = coeffs.size();
  assert(vblpmats.size() == static_cast<std::size_t>(npts));

  for (vtkIdType i = 0; i < npts; ++i)
  {
    const vtkIdType ptId = pts[i];
    double value[3] = { 0, 0, 0 };
    const auto& vblpmat = vblpmats[i];

    // interpolate field from edge/face -> nodal dof
    assert(vblpmat.size() == nDofs);
    for (std::size_t j = 0; j < nDofs; ++j)
    {
      const auto& dofBf = vblpmat[j];
      assert(vblpmat[j].size() == 3); // 3 components
      for (std::size_t k = 0; k < 3; ++k)
      {
        value[k] += dofBf[k] * coeffs[j];
      } // for k'th component of vector basis function.
    }   // for j'th edge/face
    // save new values.
    result->InsertTuple(ptId, value);
  } // for i'th point.
}

} // end anonymous namespace

VTK_ABI_NAMESPACE_BEGIN

class vtkFiniteElementFieldDistributor::vtkInternals
{
public:
  // clang-format off
  std::unordered_map<std::string, ::vtkFiniteElementSpec> femSpecs {
    { "HCURL", ::vtkFiniteElementSpec() },
    { "HDIV", ::vtkFiniteElementSpec() },
    { "HGRAD", ::vtkFiniteElementSpec() }
  };
  // clang-format on
  void InitializeReferenceElement(int order);

  void Allocate(vtkPoints* newPoints, vtkCellArray* newCells, vtkUnsignedCharArray* newCellTypes,
    vtkPointData* hGradFields, vtkPointData* hCurlFields, vtkPointData* hDivFields,
    vtkUnstructuredGrid* elements);

  //  takes a continuous mesh and explodes the point set such that each element has
  // its own collection of points unshared by any other element. This also
  // converts the mesh into potentially a higher order mesh if the DG fields require it
  void ExplodeCell(vtkIdType cellId, vtkPoints* oldPoints, vtkPoints* newPoints,
    vtkCellArray* oldCells, vtkCellArray* newCells, vtkUnsignedCharArray* newCellTypes,
    vtkPointData* oldPd, vtkPointData* newPd, vtkCellData* oldCd, vtkPointData* hGradFields);

  // Interpolates edge -> nodal dofs.
  // Interpolates face -> nodal dofs.
  void InterpolateCellToNodes(vtkIdType cellId, vtkCellArray* newCells, vtkPoints* newPoints,
    vtkCellData* oldCd, vtkPointData* hCurlFields, vtkPointData* hDivFields);

  // clear the three slots of femSpecs.
  void ResetFemSpecs();

private:
  // for convenient access to spec
  ::vtkFiniteElementSpec& hCurlSpec() { return this->femSpecs["HCURL"]; }
  ::vtkFiniteElementSpec& hDivSpec() { return this->femSpecs["HDIV"]; }
  ::vtkFiniteElementSpec& hGradSpec() { return this->femSpecs["HGRAD"]; }

  void AllocateGeometry(vtkPoints* newPoints, vtkIdType maxCellSize, vtkCellArray* newCells,
    vtkUnsignedCharArray* newCellTypes, vtkIdType numCells);

  void AllocateFields(vtkPointData* hGradFields, vtkPointData* hCurlFields,
    vtkPointData* hDivFields, vtkUnstructuredGrid* elements, vtkIdType maxNumPoints);

  static void ExplodeDGHGradCellCenteredField(vtkCellData* inCd, vtkPointData* outPd,
    const char* name, vtkIdType cellId, vtkIdType npts, const vtkIdType* pts,
    const std::vector<int>& orderingTransform);

  void ExplodeLinearCell(vtkIdType cellId, vtkPoints* oldPoints, vtkPoints* newPoints,
    vtkCellArray* oldCells, vtkCellArray* newCells, vtkUnsignedCharArray* newCellTypes,
    vtkPointData* oldPd, vtkPointData* newPd);

  void ExplodeHigherOrderCell(vtkIdType cellId, vtkPoints* oldPoints, vtkPoints* newPoints,
    vtkCellArray* oldCells, vtkCellArray* newCells, vtkUnsignedCharArray* newCellTypes,
    vtkPointData* oldPd, vtkPointData* newPd, int nComps);

  std::vector<double> GetLagrangePCoords(VTKCellType cellType, vtkIdType npts);

  vtkVectorBasisLagrangeProducts Vblps;
  VTKCellType RefElement = VTK_EMPTY_CELL;
  std::vector<double> CachedParametricCoordinates;
  int Order = 0;
  vtkNew<vtkDoubleArray> weights; // resized to maxCellSize in AllocateGeometry. Use it as you wish.
  // typed vtkCell instances allows easy access to parametric coordinates, edges, faces, ...
  vtkNew<vtkHexahedron> hex;
  vtkNew<vtkLine> line;
  vtkNew<vtkQuad> quad;
  vtkNew<vtkTriangle> tri;
  vtkNew<vtkTetra> tet;
  vtkNew<vtkWedge> wedge;
  vtkNew<vtkLagrangeHexahedron> lagrangeHex;
  vtkNew<vtkLagrangeCurve> lagrangeCurve;
  vtkNew<vtkLagrangeQuadrilateral> lagrangeQuad;
  vtkNew<vtkLagrangeTriangle> lagrangeTri;
  vtkNew<vtkLagrangeTetra> lagrangeTet;
  vtkNew<vtkLagrangeWedge> lagrangeWedge;
};

//----------------------------------------------------------------------------
void vtkFiniteElementFieldDistributor::vtkInternals::InitializeReferenceElement(int order)
{
  std::set<VTKCellType> cellTypes;
  cellTypes.insert(this->hCurlSpec().RefElement);
  cellTypes.insert(this->hDivSpec().RefElement);
  cellTypes.insert(this->hGradSpec().RefElement);
  cellTypes.erase(VTK_EMPTY_CELL);
  this->RefElement = cellTypes.size() == 1 ? *(cellTypes.begin()) : VTK_EMPTY_CELL;
  this->Order = order;
}

//----------------------------------------------------------------------------
void vtkFiniteElementFieldDistributor::vtkInternals::ResetFemSpecs()
{
  for (auto& femSpec : this->femSpecs)
  {
    femSpec.second = ::vtkFiniteElementSpec();
  }
  this->RefElement = VTK_EMPTY_CELL;
  this->Order = 0;
}

//----------------------------------------------------------------------------
void vtkFiniteElementFieldDistributor::vtkInternals::AllocateGeometry(vtkPoints* newPoints,
  vtkIdType maxCellSize, vtkCellArray* newCells, vtkUnsignedCharArray* newCellTypes,
  vtkIdType numCells)
{
  const vtkIdType maxNumPoints = numCells * maxCellSize;
  newCellTypes->SetNumberOfComponents(1);
  newCellTypes->SetNumberOfValues(numCells);
  newCells->AllocateEstimate(numCells, maxCellSize);
  newPoints->Allocate(maxNumPoints);
  this->weights->SetNumberOfValues(maxCellSize);
}

//----------------------------------------------------------------------------
void vtkFiniteElementFieldDistributor::vtkInternals::AllocateFields(vtkPointData* hGradFields,
  vtkPointData* hCurlFields, vtkPointData* hDivFields, vtkUnstructuredGrid* elements,
  vtkIdType maxNumPoints)
{
  vtkCellData* elemCd = elements->GetCellData();

  // Prepare HGRAD fields
  for (const auto& field : this->hGradSpec().Fields)
  {
    const char* name = field.c_str();
    vtkDataArray* inArr = elemCd->GetArray(name);
    if (inArr == nullptr)
    {
      vtkLog(WARNING, << "H(Grad) field - " << name << " does not exist!");
      continue;
    }
    auto arr = vtk::TakeSmartPointer(::InitializeNewArray(inArr, name, 1, 0));
    arr->Allocate(maxNumPoints);
    hGradFields->AddArray(arr);
  }
  // The new nodal form of HCurl fields will go into point data.
  for (const auto& fieldName : this->hCurlSpec().Fields)
  {
    const std::string& name = ::GetEdgeCoefficientArrayName(fieldName);
    vtkDataArray* inArr = elemCd->GetArray(name.c_str());
    if (inArr == nullptr)
    {
      vtkLog(WARNING, << "H(Curl) field - " << name << " does not exist!");
      continue;
    }
    auto arr = vtk::TakeSmartPointer(::InitializeNewArray(inArr, fieldName, 3, 0));
    arr->Allocate(maxNumPoints);
    hCurlFields->AddArray(arr);
  }
  // The new nodal form of HDiv fields will go into point data.
  for (const auto& fieldName : this->hDivSpec().Fields)
  {
    const std::string& name = ::GetFaceCoefficientArrayName(fieldName);
    vtkDataArray* inArr = elemCd->GetArray(name.c_str());
    if (inArr == nullptr)
    {
      vtkLog(WARNING, << "H(Div) field - " << name << " does not exist!");
      continue;
    }
    auto arr = vtk::TakeSmartPointer(::InitializeNewArray(inArr, fieldName, 3, 0));
    arr->Allocate(maxNumPoints);
    hDivFields->AddArray(arr);
  }
}

//----------------------------------------------------------------------------
void vtkFiniteElementFieldDistributor::vtkInternals::Allocate(vtkPoints* newPoints,
  vtkCellArray* newCells, vtkUnsignedCharArray* newCellTypes, vtkPointData* hGradFields,
  vtkPointData* hCurlFields, vtkPointData* hDivFields, vtkUnstructuredGrid* elements)
{
  if (elements == nullptr)
  {
    return;
  }
  if (elements->GetCells() == nullptr)
  {
    return;
  }

  const vtkIdType nCells = elements->GetNumberOfCells();
  const vtkIdType maxCellSize = elements->GetCells()->GetMaxCellSize();
  const vtkIdType maxNpts = nCells * maxCellSize;
  this->AllocateGeometry(newPoints, maxCellSize, newCells, newCellTypes, nCells);
  this->AllocateFields(hGradFields, hCurlFields, hDivFields, elements, maxNpts);
}

//----------------------------------------------------------------------------
std::vector<double> vtkFiniteElementFieldDistributor::vtkInternals::GetLagrangePCoords(
  VTKCellType cellType, vtkIdType npts)
{
  vtkCell* cell = nullptr;
  switch (cellType)
  {
    case VTK_HEXAHEDRON:
      this->lagrangeHex->SetUniformOrderFromNumPoints(npts);
      cell = this->lagrangeHex;
      break;
    case VTK_QUAD:
      this->lagrangeQuad->SetUniformOrderFromNumPoints(npts);
      cell = this->lagrangeQuad;
      break;
    case VTK_TETRA:
      cell = this->lagrangeTet;
      break;
    case VTK_TRIANGLE:
      cell = this->lagrangeTri;
      break;
    case VTK_WEDGE:
      cell = this->lagrangeWedge;
      break;
    default:
      break;
  }
  if (cell != nullptr)
  {
    cell->PointIds->SetNumberOfIds(npts);
    cell->Points->SetNumberOfPoints(npts);
    cell->Initialize();
    double* pCoords = cell->GetParametricCoords();
    return std::vector<double>(pCoords, pCoords + npts * 3);
  }
  else
  {
    return {};
  }
}

//----------------------------------------------------------------------------
void vtkFiniteElementFieldDistributor::vtkInternals::ExplodeCell(vtkIdType cellId,
  vtkPoints* oldPoints, vtkPoints* newPoints, vtkCellArray* oldCells, vtkCellArray* newCells,
  vtkUnsignedCharArray* newCellTypes, vtkPointData* oldPd, vtkPointData* newPd, vtkCellData* oldCd,
  vtkPointData* hGradFields)
{
  // loop over cell connectivity, redo the connectivity s.t each cell is
  // disconnected from other cells and then copy associated points into
  // the point array.
  if (this->Order == 1)
  {
    this->ExplodeLinearCell(
      cellId, oldPoints, newPoints, oldCells, newCells, newCellTypes, oldPd, newPd);
  }
  else
  {
    // Determine the order from no. of components in HGrad DG field arrays.
    std::unordered_set<int> nCompsSet;
    for (const auto& field : this->hGradSpec().Fields)
    {
      const char* name = field.c_str();
      vtkDataArray* arr = oldCd->GetArray(name);
      if (arr != nullptr)
      {
        const int nComps = arr->GetNumberOfComponents();
        nCompsSet.insert(nComps);
      }
    }
    if (nCompsSet.size() != 1)
    {
      vtkLog(WARNING,
        << "Invalid number of components for HGrad DG fields. Unable to determine order of cell "
        << cellId);
      return;
    }

    int nComps = *(nCompsSet.begin());
    this->ExplodeHigherOrderCell(
      cellId, oldPoints, newPoints, oldCells, newCells, newCellTypes, oldPd, newPd, nComps);
  }

  if (this->hGradSpec().Fields.empty())
  {
    return;
  }
  // explode n-component cell centered HGrad DG (Discontinuous Galerkin) field from cell -> nodes.
  vtkIdType newNpts = 0;
  const vtkIdType* newPts = nullptr;
  newCells->GetCellAtId(cellId, newNpts, newPts);
  // the field components follow ioss element ordering.
  auto ordering = ::GetIOSSTransformation(this->RefElement, newNpts);
  // ioss elements are 1-indexed. transform to 0-indexed lists.
  std::transform(
    ordering.cbegin(), ordering.cend(), ordering.begin(), [](vtkIdType val) { return val - 1; });
  // explode HGrad dg fields with the transformation.
  for (const auto& field : this->hGradSpec().Fields)
  {
    const char* name = field.c_str();
    this->ExplodeDGHGradCellCenteredField(
      oldCd, hGradFields, name, cellId, newNpts, newPts, ordering);
  }
}

//----------------------------------------------------------------------------
void vtkFiniteElementFieldDistributor::vtkInternals::ExplodeLinearCell(vtkIdType cellId,
  vtkPoints* oldPoints, vtkPoints* newPoints, vtkCellArray* oldCells, vtkCellArray* newCells,
  vtkUnsignedCharArray* newCellTypes, vtkPointData* oldPd, vtkPointData* newPd)
{
  vtkIdType oldNpts = 0, ind = newPoints->GetNumberOfPoints();
  const vtkIdType* oldPts = nullptr;
  double coord[3] = {};

  oldCells->GetCellAtId(cellId, oldNpts, oldPts);
  newCellTypes->SetValue(cellId, this->RefElement);
  newCells->InsertNextCell(oldNpts);

  for (vtkIdType i = 0; i < oldNpts; ++i, ++ind)
  {
    const auto& oldId = oldPts[i];
    oldPoints->GetPoint(oldId, coord);
    newPoints->InsertPoint(ind, coord);
    newCells->InsertCellPoint(ind);
    // copy over the non-dg fields from old -> new point data
    newPd->CopyData(oldPd, oldId, ind);
  }
}

//----------------------------------------------------------------------------
void vtkFiniteElementFieldDistributor::vtkInternals::ExplodeHigherOrderCell(vtkIdType cellId,
  vtkPoints* oldPoints, vtkPoints* newPoints, vtkCellArray* oldCells, vtkCellArray* newCells,
  vtkUnsignedCharArray* newCellTypes, vtkPointData* oldPd, vtkPointData* newPd, int nComps)
{
  vtkNonLinearCell* nonLinCell = nullptr;
  vtkCell* linearCell = nullptr;
  vtkNew<vtkIdList> oldPtIds;
  oldCells->GetCellAtId(cellId, oldPtIds);

  const vtkIdType& oldNpts = oldPtIds->GetNumberOfIds();
  vtkIdType newNpts = 0;

  switch (this->RefElement)
  {
    case VTK_LINE:
      switch (nComps)
      {
        case 3:
        case 4:
          // bump to VTK_LAGRANGE_CURVE order 2
          newNpts = (oldNpts != nComps) ? nComps : oldNpts;
          nonLinCell = this->lagrangeCurve;
          linearCell = this->line;
          break;
        default:
          vtkLog(WARNING, << "Unsupported number of components in HGRAD field for cell - VTK_LINE."
                          << "Supported: One of 3, 4 "
                          << "Got: " << nComps);
          break;
      }
      break;
    case VTK_TRIANGLE:
      switch (nComps)
      {
        case 6:
        case 10:
          // bump to VTK_LAGRANGE_TRIANGLE order 2
          newNpts = (oldNpts != nComps) ? nComps : oldNpts;
          nonLinCell = this->lagrangeTri;
          linearCell = this->tri;
          break;
        default:
          vtkLog(
            WARNING, << "Unsupported number of components in HGRAD field for cell - VTK_TRIANGLE."
                     << "Supported: One of 6, 10"
                     << "Got: " << nComps);
          break;
      }
      break;
    case VTK_QUAD:
      switch (nComps)
      {
        case 9:
        case 16:
          // bump to VTK_LAGRANGE_QUADRILATERAL order n
          newNpts = (oldNpts != nComps) ? nComps : oldNpts;
          this->lagrangeQuad->SetUniformOrderFromNumPoints(newNpts);
          nonLinCell = this->lagrangeQuad;
          linearCell = this->quad;
          break;
        default:
          vtkLog(WARNING, << "Unsupported number of components in HGRAD field for cell - VTK_QUAD."
                          << "Supported: One of 9, 16 "
                          << "Got: " << nComps);
          break;
      }
      break;
    case VTK_TETRA:
      switch (nComps)
      {
        case 10:
        case 11:
        case 15:
          // bump to VTK_LAGRANGE_TETRAHEDRON order n
          newNpts = (oldNpts != nComps) ? nComps : oldNpts;
          nonLinCell = this->lagrangeTet;
          linearCell = this->tet;
          break;
        default:
          vtkLog(WARNING, << "Unsupported number of components in HGRAD field for cell - VTK_TETRA."
                          << "Supported: One of 10, 11, 15 "
                          << "Got: " << nComps);
          break;
      }
      break;
    case VTK_PYRAMID:
      // vtk does not have vtkHigherOrderPyramid.
      switch (nComps)
      {
        case 13:
        case 14:
        case 19:
        default:
          vtkLog(
            WARNING, << "Unsupported number of components in HGRAD field for cell - VTK_PYRAMID."
                     << "Supported: None "
                     << "Got: " << nComps);
          break;
      }
      break;
    case VTK_WEDGE:
      switch (nComps)
      {
        case 15:
        case 18:
        case 21:
          // bump to VTK_LAGRANGE_WEDGE order n
          newNpts = (oldNpts != nComps) ? nComps : oldNpts;
          this->lagrangeWedge->SetUniformOrderFromNumPoints(newNpts);
          nonLinCell = this->lagrangeWedge;
          linearCell = this->wedge;
          break;
        default:
          vtkLog(WARNING, << "Unsupported number of components in HGRAD field for cell - VTK_WEDGE."
                          << "Supported: 15, 18, 21 "
                          << "Got: " << nComps);
          break;
      }
      break;
    case VTK_HEXAHEDRON:
      switch (nComps)
      {
        case 20:
        case 27:
          // bump to VTK_LAGRANGE_HEXAHEDRON order n
          newNpts = (oldNpts != nComps) ? nComps : oldNpts;
          this->lagrangeHex->SetUniformOrderFromNumPoints(newNpts);
          nonLinCell = this->lagrangeHex;
          linearCell = this->hex;
          break;
        default:
          vtkLog(
            WARNING, << "Unsupported number of components in HGRAD field for cell - VTK_HEXAHEDRON."
                     << "Supported: 20, 27 "
                     << "Got: " << nComps);
          break;
      }
      break;
    default:
      vtkLog(WARNING, << "Unsupported higher order cell: " << this->RefElement);
      break;
  }

  if (nonLinCell != nullptr)
  {
    double* pCoords = nullptr;
    double coord[3] = {};
    int subId = 0;
    const vtkIdType* oldPts = oldPtIds->GetPointer(0);
    vtkIdType ind = newPoints->GetNumberOfPoints();

    newCells->InsertNextCell(newNpts);
    newCellTypes->SetValue(cellId, nonLinCell->GetCellType());

    // insert points from old cell.
    for (unsigned short i = 0; i < oldNpts; ++i, ++ind)
    {
      const auto& oldId = oldPts[i];
      oldPoints->GetPoint(oldId, coord);
      newPoints->InsertPoint(ind, coord);
      newCells->InsertCellPoint(ind);
      // copy over the non-dg fields from old -> new point data
      newPd->CopyData(oldPd, oldId, ind);
    }

    // need to construct a higher order cell from a linear cell.
    if (linearCell != nullptr)
    {
      linearCell->Initialize(oldNpts, oldPts, oldPoints);
      // add points at mid-edge, mid-face locations or at volume center.
      nonLinCell->Points->SetNumberOfPoints(newNpts);
      nonLinCell->PointIds->SetNumberOfIds(newNpts);
      nonLinCell->Initialize();
      if (this->weights->GetNumberOfValues() < oldNpts)
      {
        // resize to adjust for bigger cells as needed.
        this->weights->SetNumberOfValues(oldNpts);
      }
      this->weights->FillValue(0.0);
      pCoords = nonLinCell->GetParametricCoords();
      for (unsigned short i = oldNpts; i < newNpts; ++i, ++ind)
      {
        linearCell->EvaluateLocation(
          subId, &pCoords[3 * static_cast<ptrdiff_t>(i)], coord, this->weights->GetPointer(0));
        newPoints->InsertPoint(ind, coord);
        newCells->InsertCellPoint(ind);
        // interpolate the non-dg fields from old -> new point data
        newPd->InterpolatePoint(oldPd, ind, oldPtIds, this->weights->GetPointer(0));
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkFiniteElementFieldDistributor::vtkInternals::ExplodeDGHGradCellCenteredField(
  vtkCellData* inCd, vtkPointData* outPd, const char* name, vtkIdType cellId, vtkIdType npts,
  const vtkIdType* pts, const std::vector<int>& orderingTransform)
{
  vtkDataArray* const inArr = inCd->GetArray(name);
  vtkDataArray* const outArr = outPd->GetArray(name);
  if (inArr == nullptr || outArr == nullptr)
  {
    vtkLog(WARNING, << "Invalid HGRAD DG field data. Cannot find array : " << name);
    return;
  }

  if (inArr->GetNumberOfComponents() == npts)
  {
    if (static_cast<int>(orderingTransform.size()) == npts)
    {
      for (vtkIdType i = 0; i < npts; ++i)
      {
        double value =
          inArr->GetComponent(cellId, orderingTransform[i]); // get the transformed i'th component
        outArr->InsertComponent(pts[i], 0, value);
      }
    }
    else
    { // fallback to naïve ordering
      for (vtkIdType i = 0; i < npts; ++i)
      {
        double value = inArr->GetComponent(cellId, i);
        outArr->InsertComponent(pts[i], 0, value);
      }
    }
  }
  else
  {
    vtkLog(WARNING, << "HGRAD field(" << name << ") component mismatch. CellSize(" << npts
                    << ") != nComps(" << inArr->GetNumberOfComponents() << ")");
  }
}

//----------------------------------------------------------------------------
void vtkFiniteElementFieldDistributor::vtkInternals::InterpolateCellToNodes(vtkIdType cellId,
  vtkCellArray* newCells, vtkPoints* newPoints, vtkCellData* oldCd, vtkPointData* hCurlFields,
  vtkPointData* hDivFields)
{
  // we will interpolate onto the points found at new point ids. (from cell explosion)
  const vtkIdType* newPts = nullptr;
  vtkIdType newNpts = 0;
  newCells->GetCellAtId(cellId, newNpts, newPts);
  if (this->Vblps.RequiresInitialization(this->RefElement, newNpts))
  {
    auto pCoords = this->GetLagrangePCoords(this->RefElement, newNpts);
    // for all others, need to shift center of element to (0,0,0)
    if (this->RefElement != VTK_TRIANGLE && this->RefElement != VTK_TETRA)
    {
      std::transform(pCoords.cbegin(), pCoords.cend(), pCoords.begin(),
        [](const double& val) -> double { return 2 * (val - 0.5); });
    }
    this->Vblps.Initialize(this->RefElement, pCoords.data(), newNpts);
    this->CachedParametricCoordinates = pCoords;
  }
  this->Vblps.ComputeJacobian(
    this->RefElement, this->CachedParametricCoordinates.data(), newNpts, newPts, newPoints);

  for (const auto& fieldName : this->hCurlSpec().Fields)
  {
    std::vector<double> coeffs = ::GetEdgeAttributes(fieldName, oldCd, cellId);
    if (coeffs.empty())
    {
      continue;
    }
    vtkDataArray* outArr = hCurlFields->GetArray(fieldName.c_str());
    const auto refVblpMat = this->Vblps.GetVblp(::SpaceType::HCurl, this->RefElement);
    const auto jacMat = this->Vblps.GetJacobianMatrices(this->RefElement);
    if (refVblpMat != nullptr && jacMat != nullptr)
    {
      const auto physVblpMat =
        this->Vblps.ReferenceToPhysical(*refVblpMat, *jacMat, SpaceType::HCurl);
      ::InterpolateToNodes(physVblpMat, coeffs, newNpts, newPts, outArr);
    }
  }

  for (const auto& fieldName : this->hDivSpec().Fields)
  {
    std::vector<double> coeffs;
    if (this->RefElement == VTK_QUAD || this->RefElement == VTK_TRIANGLE)
    {
      coeffs = ::GetEdgeAttributes(fieldName, oldCd, cellId);
    }
    else
    {
      coeffs = ::GetFaceAttributes(fieldName, oldCd, cellId);
    }
    if (coeffs.empty())
    {
      continue;
    }
    vtkDataArray* outArr = hDivFields->GetArray(fieldName.c_str());
    const auto refVblpMat = this->Vblps.GetVblp(::SpaceType::HDiv, this->RefElement);
    const auto jacMat = this->Vblps.GetJacobianMatrices(this->RefElement);
    if (refVblpMat != nullptr && jacMat != nullptr)
    {
      const auto physVblpMat =
        this->Vblps.ReferenceToPhysical(*refVblpMat, *jacMat, SpaceType::HDiv);
      ::InterpolateToNodes(physVblpMat, coeffs, newNpts, newPts, outArr);
    }
  }
}

vtkStandardNewMacro(vtkFiniteElementFieldDistributor);

//----------------------------------------------------------------------------
vtkFiniteElementFieldDistributor::vtkFiniteElementFieldDistributor()
  : Internals(new vtkFiniteElementFieldDistributor::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkFiniteElementFieldDistributor::~vtkFiniteElementFieldDistributor() = default;

//----------------------------------------------------------------------------
void vtkFiniteElementFieldDistributor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkFiniteElementFieldDistributor::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Internals->ResetFemSpecs();

  vtkPartitionedDataSetCollection* input = vtkPartitionedDataSetCollection::GetData(inputVector[0]);
  vtkPartitionedDataSetCollection* output = vtkPartitionedDataSetCollection::GetData(outputVector);

  // Look for special string array containing information records.
  vtkFieldData* fd = input->GetFieldData();
  vtkStringArray* infoRecords = vtkStringArray::SafeDownCast(fd->GetAbstractArray(infoRecordName));
  if (infoRecords == nullptr)
  {
    vtkErrorMacro(<< "Failed to find a string array - " << infoRecordName);
    return 0;
  }

  // Parse the information records.
  int refElementOrder = 0;
  std::unordered_set<std::string> elementBlockNames;
  for (vtkIdType i = 0; i < infoRecords->GetNumberOfValues(); ++i)
  {
    const auto& record = infoRecords->GetValue(i);
    ::vtkFiniteElementSpec* femSpec = nullptr;

    const std::vector<std::string> data = ::Split(record, "::");
    // Examples:
    // "HDIV::eblock-0_0_0::CG::basis::Intrepid2_HDIV_HEX_I1_FEM"
    //    0       1         2     3              4
    //
    // "HGRAD::eblock-0_0::DG::basis::Intrepid2_HGRAD_QUAD_C2_FEM"
    //    0       1        2     3              4
    //
    // "HCURL::eblock-0_0_0::CG::basis::Intrepid2_HCURL_HEX_I1_FEM"
    //    0       1          2     3              4
    //
    // "HCURL::eblock-0_0_0::CG::field::E_Field"
    //    0       1          2     3      4
    if (data.size() < 5)
    {
      continue;
    }
    // within this context, an entity is either a basis or a field.
    const std::string& basisType = data[0];
    const std::string& blockName = data[1];
    const std::string& galerkinType = data[2];
    const std::string& entityType = data[3];
    const std::string& entityName = data[4];
    // Look for valid FEM element callouts.
    if (!(basisType == "HCURL" || basisType == "HDIV" || basisType == "HGRAD"))
    {
      continue;
    }
    // our concern is only those element blocks which have FEM element type callouts.
    elementBlockNames.insert(blockName);

    femSpec = &(this->Internals->femSpecs[basisType]);

    if (entityType == "basis")
    {
      const auto& intrepidName = entityName;
      const std::vector<std::string> nameParts = ::Split(intrepidName, "_");
      // Examples:
      // "Intrepid2_HCURL_HEX_I1_FEM"
      //      0       1    2  3   4
      const int currentBasisOrder = nameParts[3][1] - '0';
      if (galerkinType == "CG")
      {
        refElementOrder = currentBasisOrder > refElementOrder ? currentBasisOrder : refElementOrder;
      }
      else if (galerkinType == "DG")
      {
        refElementOrder = currentBasisOrder > refElementOrder ? currentBasisOrder : refElementOrder;
      }
      const auto& elementName = nameParts[2];
      if (elementName == "HEX")
      {
        femSpec->RefElement = VTK_HEXAHEDRON;
      }
      else if (elementName == "LINE")
      {
        femSpec->RefElement = VTK_LINE;
      }
      else if (elementName == "PYR")
      {
        femSpec->RefElement = VTK_PYRAMID;
      }
      else if (elementName == "QUAD")
      {
        femSpec->RefElement = VTK_QUAD;
      }
      else if (elementName == "TET")
      {
        femSpec->RefElement = VTK_TETRA;
      }
      else if (elementName == "TRI")
      {
        femSpec->RefElement = VTK_TRIANGLE;
      }
      else if (elementName == "WEDGE")
      {
        femSpec->RefElement = VTK_WEDGE;
      }
    }
    else if (entityType == "field" && femSpec != nullptr)
    {
      // these fields will be attached to a basis.
      if (galerkinType == "CG" && basisType != "HGRAD")
      {
        femSpec->Fields.insert(entityName);
      }
      else if (galerkinType == "DG" && basisType == "HGRAD")
      {
        femSpec->Fields.insert(entityName);
      }
    }
  }
  if (elementBlockNames.empty())
  {
    vtkErrorMacro(<< "Failed to find element blocks!");
    return 0;
  }

  this->Internals->InitializeReferenceElement(refElementOrder);

  bool abortNow = false;
  unsigned int pdsIdx = 0;
  for (const auto& blockName : elementBlockNames)
  {
    if (abortNow)
    {
      break;
    }
    vtkPartitionedDataSet* elementsPds = nullptr;
    // Find an element block.
    if (!blockName.empty())
    {
      elementsPds = ::GetNamedPartitionedDataSet(blockName, input);
    }
    if (elementsPds == nullptr)
    {
      continue;
    }

    // TODO: mpi-fy this thing..
    const unsigned int numParts = elementsPds->GetNumberOfPartitions();
    for (unsigned int partIdx = 0; partIdx < numParts && !abortNow; ++partIdx)
    {
      vtkUnstructuredGrid* elements =
        vtkUnstructuredGrid::SafeDownCast(elementsPds->GetPartition(partIdx));
      if (elements == nullptr || elements->GetNumberOfPoints() == 0 ||
        elements->GetNumberOfCells() == 0)
      {
        continue;
      }

      vtkPoints* oldPoints = elements->GetPoints();
      vtkCellArray* oldCells = elements->GetCells();

      // peek at the elements block to allocate appropriate output.
      vtkNew<vtkUnstructuredGrid> newMesh;
      vtkNew<vtkUnsignedCharArray> newCellTypes;
      vtkNew<vtkPointData> hGradFields, hCurlFields, hDivFields;
      auto newPoints = vtk::TakeSmartPointer(oldPoints->NewInstance());
      auto newCells = vtk::TakeSmartPointer(oldCells->NewInstance());
      this->Internals->Allocate(
        newPoints, newCells, newCellTypes, hGradFields, hCurlFields, hDivFields, elements);

      // copy/interpolate dataset attributes.
      vtkCellData *oldCd = elements->GetCellData(), *newCd = newMesh->GetCellData();
      vtkPointData *oldPd = elements->GetPointData(), *newPd = newMesh->GetPointData();
      vtkFieldData *oldFd = elements->GetFieldData(), *newFd = newMesh->GetFieldData();
      // when we bump cell order, new points are created. requires weighted interpolation for
      // CG (Continuous Galerkin) point data arrays.
      newPd->InterpolateAllocate(oldPd);
      newCd->CopyAllocate(oldCd);
      newFd->DeepCopy(oldFd);

      // explode geometry, interpolate fields.
      const double progressGranularity = 0.1;
      const vtkIdType nCells = oldCells->GetNumberOfCells();
      const vtkIdType reportEveryNCells = progressGranularity * nCells;
      for (vtkIdType c = 0; c < nCells && !abortNow; ++c)
      {
        this->Internals->ExplodeCell(c, oldPoints, newPoints, oldCells, newCells, newCellTypes,
          oldPd, newPd, oldCd, hGradFields);
        this->Internals->InterpolateCellToNodes(
          c, newCells, newPoints, oldCd, hCurlFields, hDivFields);

        newCd->CopyData(oldCd, c, c);

        if (c % reportEveryNCells == 0)
        {
          abortNow = this->CheckAbort();
          this->UpdateProgress(static_cast<double>(c) / nCells);
        }
      } // for each cell
      if (abortNow)
      {
        continue;
      }

      // Finalize geometry, topology of output mesh.
      newMesh->SetPoints(newPoints);
      newMesh->SetCells(newCellTypes, newCells);
      output->SetPartition(pdsIdx, partIdx, newMesh);
      output->GetMetaData(pdsIdx)->Set(vtkCompositeDataSet::NAME(), blockName.c_str());

      // Copy over the hgrad/hcurl/hdiv fields into output point data.
      for (int i = 0; i < hGradFields->GetNumberOfArrays(); ++i)
      {
        if (vtkDataArray* arr = hGradFields->GetArray(i))
        {
          if (arr->GetNumberOfTuples())
          {
            const char* name = hGradFields->GetArrayName(i);
            newPd->AddArray(arr);
            // remove the HGrad cell-centered DG arrays from output.
            newCd->RemoveArray(name);
          }
        }
      }
      for (int i = 0; i < hCurlFields->GetNumberOfArrays(); ++i)
      {
        if (vtkDataArray* arr = hCurlFields->GetArray(i))
        {
          if (arr->GetNumberOfTuples())
          {
            newPd->AddArray(arr);
          }
          // remove the edge coefficient arrays from output.
          newCd->RemoveArray(::GetEdgeCoefficientArrayName(arr->GetName()).c_str());
        }
      }
      for (int i = 0; i < hDivFields->GetNumberOfArrays(); ++i)
      {
        if (vtkDataArray* arr = hDivFields->GetArray(i))
        {
          if (arr->GetNumberOfTuples())
          {
            newPd->AddArray(arr);
          }
          // remove the face coefficient arrays from output.
          newCd->RemoveArray(::GetFaceCoefficientArrayName(arr->GetName()).c_str());
        }
      }
    } // for each partition
    ++pdsIdx;
  } // for each element block
  return 1;
}
VTK_ABI_NAMESPACE_END
