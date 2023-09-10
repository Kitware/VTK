// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGInterpolateCalculator.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellMetadata.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkTypeInt64Array.h"
#include "vtkVectorOperators.h"

using namespace vtk::literals;

#define vtkDGInvokeBasis(basisMethod, shape)                                                       \
  case shape:                                                                                      \
    basisMethod<shape>(rst, basis);                                                                \
    break

#define vtkDGInvokeBasisForShapes(basisMethod, cellShape)                                          \
  switch (cellShape)                                                                               \
  {                                                                                                \
    vtkDGInvokeBasis(basisMethod, vtkDGCell::Shape::Pyramid);                                      \
    vtkDGInvokeBasis(basisMethod, vtkDGCell::Shape::Wedge);                                        \
    vtkDGInvokeBasis(basisMethod, vtkDGCell::Shape::Hexahedron);                                   \
    vtkDGInvokeBasis(basisMethod, vtkDGCell::Shape::Tetrahedron);                                  \
    vtkDGInvokeBasis(basisMethod, vtkDGCell::Shape::Quadrilateral);                                \
    vtkDGInvokeBasis(basisMethod, vtkDGCell::Shape::Triangle);                                     \
    vtkDGInvokeBasis(basisMethod, vtkDGCell::Shape::Edge);                                         \
    vtkDGInvokeBasis(basisMethod, vtkDGCell::Shape::Vertex);                                       \
    default:                                                                                       \
    {                                                                                              \
      vtkErrorMacro("Unsupported shape " << cellShape << ".");                                     \
    }                                                                                              \
    break;                                                                                         \
  }

// We turn off formatting here since the basis functions are
// copied from elsewhere and are highly-regular mathematical
// expressions with subtle symmetries that are best exposed
// with spaces to align subexpressions.
//
// clang-format off
namespace
{

// ## H(Curl) function space
template<vtkDGCell::Shape ShapeEnum>
void CGHCurlBasis(const vtkVector3d& rst, std::vector<double>& basis);

// The following basis functions are adapted from https://github.com/trilinos/Trilinos
// Hexahedron:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HCURL_HEX_I1_FEMDef.hpp
template<>
void CGHCurlBasis<vtkDGCell::Shape::Hexahedron>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0*3 + 0] = (1.0 - rst[1])*(1.0 - rst[2])/8.0;
  basis[0*3 + 1] = 0.0;
  basis[0*3 + 2] = 0.0;

  basis[1*3 + 0] = 0.0;
  basis[1*3 + 1] = (1.0 + rst[0])*(1.0 - rst[2])/8.0;
  basis[1*3 + 2] = 0.0;

  basis[2*3 + 0] = -(1.0 + rst[1])*(1.0 - rst[2])/8.0;
  basis[2*3 + 1] = 0.0;
  basis[2*3 + 2] = 0.0;

  basis[3*3 + 0] = 0.0;
  basis[3*3 + 1] = -(1.0 - rst[0])*(1.0 - rst[2])/8.0;
  basis[3*3 + 2] = 0.0;

  basis[4*3 + 0] = (1.0 - rst[1])*(1.0 + rst[2])/8.0;
  basis[4*3 + 1] = 0.0;
  basis[4*3 + 2] = 0.0;

  basis[5*3 + 0] = 0.0;
  basis[5*3 + 1] = (1.0 + rst[0])*(1.0 + rst[2])/8.0;
  basis[5*3 + 2] = 0.0;

  basis[6*3 + 0] = -(1.0 + rst[1])*(1.0 + rst[2])/8.0;
  basis[6*3 + 1] = 0.0;
  basis[6*3 + 2] = 0.0;

  basis[7*3 + 0] = 0.0;
  basis[7*3 + 1] = -(1.0 - rst[0])*(1.0 + rst[2])/8.0;
  basis[7*3 + 2] = 0.0;

  basis[8*3 + 0] = 0.0;
  basis[8*3 + 1] = 0.0;
  basis[8*3 + 2] = (1.0 - rst[0])*(1.0 - rst[1])/8.0;

  basis[9*3 + 0] = 0.0;
  basis[9*3 + 1] = 0.0;
  basis[9*3 + 2] = (1.0 + rst[0])*(1.0 - rst[1])/8.0;

  basis[10*3 + 0] = 0.0;
  basis[10*3 + 1] = 0.0;
  basis[10*3 + 2] = (1.0 + rst[0])*(1.0 + rst[1])/8.0;

  basis[11*3 + 0] = 0.0;
  basis[11*3 + 1] = 0.0;
  basis[11*3 + 2] = (1.0 - rst[0])*(1.0 + rst[1])/8.0;
}

// Tetrahedron:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HCURL_TET_I1_FEMDef.hpp
template<>
void CGHCurlBasis<vtkDGCell::Shape::Tetrahedron>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0*3 + 0] = 1.0 - rst[1] - rst[2];
  basis[0*3 + 1] = rst[0];
  basis[0*3 + 2] = rst[0];

  basis[1*3 + 0] =-rst[1];
  basis[1*3 + 1] = rst[0];
  basis[1*3 + 2] = 0.0;

  basis[2*3 + 0] = -rst[1];
  basis[2*3 + 1] = -1.0 + rst[0] + rst[2];
  basis[2*3 + 2] = -rst[1];

  basis[3*3 + 0] = rst[2];
  basis[3*3 + 1] = rst[2];
  basis[3*3 + 2] = 1.0 - rst[0] - rst[1];

  basis[4*3 + 0] =-rst[2];
  basis[4*3 + 1] = 0.0;
  basis[4*3 + 2] = rst[0];

  basis[5*3 + 0] = 0.0;
  basis[5*3 + 1] =-rst[2];
  basis[5*3 + 2] = rst[1];
}

// Wedge:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HCURL_WEDGE_I1_FEMDef.hpp
template<>
void CGHCurlBasis<vtkDGCell::Shape::Wedge>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0*3 + 0] = (1.0 - rst[2])*(1.0 - rst[1])/2.0;
  basis[0*3 + 1] = rst[0]*(1.0 - rst[2])/2.0;
  basis[0*3 + 2] = 0.0;

  basis[1*3 + 0] = rst[1]*(rst[2] - 1.0)/2.0;
  basis[1*3 + 1] = rst[0]*(1.0 - rst[2])/2.0;
  basis[1*3 + 2] = 0.0;

  basis[2*3 + 0] = rst[1]*(rst[2] - 1.0)/2.0;
  basis[2*3 + 1] = (1.0 - rst[0])*(rst[2] - 1.0)/2.0;
  basis[2*3 + 2] = 0.0;

  basis[3*3 + 0] = (1.0 - rst[1])*(1.0 + rst[2])/2.0;
  basis[3*3 + 1] = rst[0]*(1.0 + rst[2])/2.0;
  basis[3*3 + 2] = 0.0;

  basis[4*3 + 0] =-rst[1]*(1.0 + rst[2])/2.0;
  basis[4*3 + 1] = rst[0]*(1.0 + rst[2])/2.0;
  basis[4*3 + 2] = 0.0;

  basis[5*3 + 0] = -rst[1]*(1.0 + rst[2])/2.0;
  basis[5*3 + 1] = (rst[0] - 1.0)*(1.0 + rst[2])/2.0;
  basis[5*3 + 2] = 0.0;

  basis[6*3 + 0] = 0.0;
  basis[6*3 + 1] = 0.0;
  basis[6*3 + 2] = (1.0 - rst[0] - rst[1])/2.0;

  basis[7*3 + 0] = 0.0;
  basis[7*3 + 1] = 0.0;
  basis[7*3 + 2] = rst[0]/2.0;

  basis[8*3 + 0] = 0.0;
  basis[8*3 + 1] = 0.0;
  basis[8*3 + 2] = rst[1]/2.0;
}

// Quadrilateral:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HCURL_QUAD_I1_FEMDef.hpp
template<>
void CGHCurlBasis<vtkDGCell::Shape::Quadrilateral>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0*2 + 0] = (1.0 - rst[1])/4.0;
  basis[0*2 + 1] =  0.0;

  basis[1*2 + 0] =  0.0;
  basis[1*2 + 1] = (1.0 + rst[0])/4.0;

  basis[2*2 + 0] =-(1.0 + rst[1])/4.0;
  basis[2*2 + 1] =  0.0;

  basis[3*2 + 0] =  0.0;
  basis[3*2 + 1] =-(1.0 - rst[0])/4.0;
}

// Triangle:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HCURL_TRI_I1_FEMDef.hpp
template<>
void CGHCurlBasis<vtkDGCell::Shape::Triangle>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0*3 + 0] =  1.0 - rst[1];
  basis[0*3 + 1] =  rst[0];

  basis[1*3 + 0] = -rst[1];
  basis[1*3 + 1] =  rst[0];

  basis[2*3 + 0] = -rst[1];
  basis[2*3 + 1] = -1.0 + rst[0];
}

// HCurl does not support these shapes
template<>
void CGHCurlBasis<vtkDGCell::Shape::Pyramid>(const vtkVector3d&, std::vector<double>&)
{ vtkGenericWarningMacro("Pyramid cells are unsupported for HCurl."); }

template<>
void CGHCurlBasis<vtkDGCell::Shape::Edge>(const vtkVector3d&, std::vector<double>&)
{ vtkGenericWarningMacro("Line cells are unsupported for HCurl."); }

template<>
void CGHCurlBasis<vtkDGCell::Shape::Vertex>(const vtkVector3d&, std::vector<double>&)
{ vtkGenericWarningMacro("Vertex cells are unsupported for HCurl."); }

// ## H(Div) function space
template<vtkDGCell::Shape ShapeEnum>
void CGHDivBasis(const vtkVector3d& rst, std::vector<double>& basis);

// The following basis functions are adapted from https://github.com/trilinos/Trilinos
// Hexahedron:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HDIV_HEX_I1_FEMDef.hpp
template<>
void CGHDivBasis<vtkDGCell::Shape::Hexahedron>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0*3 + 0] = 0.0;
  basis[0*3 + 1] = (rst[1] - 1.0)/8.0;
  basis[0*3 + 2] = 0.0;

  basis[1*3 + 0] = (1.0 + rst[0])/8.0;
  basis[1*3 + 1] = 0.0;
  basis[1*3 + 2] = 0.0;

  basis[2*3 + 0] = 0.0;
  basis[2*3 + 1] = (1.0 + rst[1])/8.0;
  basis[2*3 + 2] = 0.0;

  basis[3*3 + 0] = (rst[0] - 1.0)/8.0;
  basis[3*3 + 1] = 0.0;
  basis[3*3 + 2] = 0.0;

  basis[4*3 + 0] = 0.0;
  basis[4*3 + 1] = 0.0;
  basis[4*3 + 2] = (rst[2] - 1.0)/8.0;

  basis[5*3 + 0] = 0.0;
  basis[5*3 + 1] = 0.0;
  basis[5*3 + 2] = (1.0 + rst[2])/8.0;
}

// Tetrahedron:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HDIV_TET_I1_FEMDef.hpp
template<>
void CGHDivBasis<vtkDGCell::Shape::Tetrahedron>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0*3 + 0] = 2.0*rst[0];
  basis[0*3 + 1] = 2.0*(rst[1] - 1.0);
  basis[0*3 + 2] = 2.0*rst[2];

  basis[1*3 + 0] = 2.0*rst[0];
  basis[1*3 + 1] = 2.0*rst[1];
  basis[1*3 + 2] = 2.0*rst[2];

  basis[2*3 + 0] = 2.0*(rst[0] - 1.0);
  basis[2*3 + 1] = 2.0*rst[1];
  basis[2*3 + 2] = 2.0*rst[2];

  basis[3*3 + 0] = 2.0*rst[0];
  basis[3*3 + 1] = 2.0*rst[1];
  basis[3*3 + 2] = 2.0*(rst[2] - 1.0);
}

// Wedge:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HDIV_WEDGE_I1_FEMDef.hpp
template<>
void CGHDivBasis<vtkDGCell::Shape::Wedge>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0*3 + 0] = rst[0]/2.0;
  basis[0*3 + 1] = (rst[1] - 1.0)/2.0;
  basis[0*3 + 2] = 0.0;

  basis[1*3 + 0] = rst[0]/2.0;
  basis[1*3 + 1] = rst[1]/2.0;
  basis[1*3 + 2] = 0.0;

  basis[2*3 + 0] = (rst[0] - 1.0)/2.0;
  basis[2*3 + 1] = rst[1]/2.0;
  basis[2*3 + 2] = 0.0;

  basis[3*3 + 0] = 0.0;
  basis[3*3 + 1] = 0.0;
  basis[3*3 + 2] = rst[2] - 1.0;

  basis[4*3 + 0] = 0.0;
  basis[4*3 + 1] = 0.0;
  basis[4*3 + 2] = 1.0 + rst[2];
}

// Quadrilateral:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HDIV_QUAD_I1_FEMDef.hpp
template<>
void CGHDivBasis<vtkDGCell::Shape::Quadrilateral>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0*2 + 0] = 0.0;
  basis[0*2 + 1] = (rst[1] - 1.0)/4.0;

  basis[1*2 + 0] = (1.0 + rst[0])/4.0;
  basis[1*2 + 1] = 0.0;

  basis[2*2 + 0] = 0.0;
  basis[2*2 + 1] = (1.0 + rst[1])/4.0;

  basis[3*2 + 0] = (rst[0] - 1.0)/4.0;
  basis[3*2 + 1] = 0.0;
}

// Triangle:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HDIV_TRI_I1_FEMDef.hpp
template<>
void CGHDivBasis<vtkDGCell::Shape::Triangle>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0*2 + 0] = rst[0];
  basis[0*2 + 1] = rst[1] - 1.0;

  basis[1*2 + 0] = rst[0];
  basis[1*2 + 1] = rst[1];

  basis[2*2 + 0] = rst[0] - 1.0;
  basis[2*2 + 1] = rst[1];
}

// HDiv does not support these shapes
template<>
void CGHDivBasis<vtkDGCell::Shape::Pyramid>(const vtkVector3d&, std::vector<double>&)
{ vtkGenericWarningMacro("Pyramid cells are unsupported for HDiv."); }

template<>
void CGHDivBasis<vtkDGCell::Shape::Edge>(const vtkVector3d&, std::vector<double>&)
{ vtkGenericWarningMacro("Line cells are unsupported for HDiv."); }

template<>
void CGHDivBasis<vtkDGCell::Shape::Vertex>(const vtkVector3d&, std::vector<double>&)
{ vtkGenericWarningMacro("Vertex cells are unsupported for HDiv."); }

// ## H(Grad) function space (Lagrange)
template<vtkDGCell::Shape ShapeEnum>
void DGHGradBasis(const vtkVector3d& rst, std::vector<double>& basis);

template<vtkDGCell::Shape ShapeEnum>
void DGHGradGradient(const vtkVector3d& rst, std::vector<double>& jacobian);

// The following basis functions are adapted from https://github.com/trilinos/Trilinos
// Hexahedron:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HGRAD_HEX_I1_FEMDef.hpp
template<>
void DGHGradBasis<vtkDGCell::Shape::Hexahedron>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0] = (1.0 - rst[0])*(1.0 - rst[1])*(1.0 - rst[2])/8.0;
  basis[1] = (1.0 + rst[0])*(1.0 - rst[1])*(1.0 - rst[2])/8.0;
  basis[2] = (1.0 + rst[0])*(1.0 + rst[1])*(1.0 - rst[2])/8.0;
  basis[3] = (1.0 - rst[0])*(1.0 + rst[1])*(1.0 - rst[2])/8.0;

  basis[4] = (1.0 - rst[0])*(1.0 - rst[1])*(1.0 + rst[2])/8.0;
  basis[5] = (1.0 + rst[0])*(1.0 - rst[1])*(1.0 + rst[2])/8.0;
  basis[6] = (1.0 + rst[0])*(1.0 + rst[1])*(1.0 + rst[2])/8.0;
  basis[7] = (1.0 - rst[0])*(1.0 + rst[1])*(1.0 + rst[2])/8.0;
}

template<>
void DGHGradGradient<vtkDGCell::Shape::Hexahedron>(const vtkVector3d& rst, std::vector<double>& jacobian)
{
  jacobian[0*3 + 0] = -(1.0 - rst[1])*(1.0 - rst[2])/8.0;
  jacobian[0*3 + 1] = -(1.0 - rst[0])*(1.0 - rst[2])/8.0;
  jacobian[0*3 + 2] = -(1.0 - rst[0])*(1.0 - rst[1])/8.0;

  jacobian[1*3 + 0] =  (1.0 - rst[1])*(1.0 - rst[2])/8.0;
  jacobian[1*3 + 1] = -(1.0 + rst[0])*(1.0 - rst[2])/8.0;
  jacobian[1*3 + 2] = -(1.0 + rst[0])*(1.0 - rst[1])/8.0;

  jacobian[2*3 + 0] =  (1.0 + rst[1])*(1.0 - rst[2])/8.0;
  jacobian[2*3 + 1] =  (1.0 + rst[0])*(1.0 - rst[2])/8.0;
  jacobian[2*3 + 2] = -(1.0 + rst[0])*(1.0 + rst[1])/8.0;

  jacobian[3*3 + 0] = -(1.0 + rst[1])*(1.0 - rst[2])/8.0;
  jacobian[3*3 + 1] =  (1.0 - rst[0])*(1.0 - rst[2])/8.0;
  jacobian[3*3 + 2] = -(1.0 - rst[0])*(1.0 + rst[1])/8.0;

  jacobian[4*3 + 0] = -(1.0 - rst[1])*(1.0 + rst[2])/8.0;
  jacobian[4*3 + 1] = -(1.0 - rst[0])*(1.0 + rst[2])/8.0;
  jacobian[4*3 + 2] =  (1.0 - rst[0])*(1.0 - rst[1])/8.0;

  jacobian[5*3 + 0] =  (1.0 - rst[1])*(1.0 + rst[2])/8.0;
  jacobian[5*3 + 1] = -(1.0 + rst[0])*(1.0 + rst[2])/8.0;
  jacobian[5*3 + 2] =  (1.0 + rst[0])*(1.0 - rst[1])/8.0;

  jacobian[6*3 + 0] =  (1.0 + rst[1])*(1.0 + rst[2])/8.0;
  jacobian[6*3 + 1] =  (1.0 + rst[0])*(1.0 + rst[2])/8.0;
  jacobian[6*3 + 2] =  (1.0 + rst[0])*(1.0 + rst[1])/8.0;

  jacobian[7*3 + 0] = -(1.0 + rst[1])*(1.0 + rst[2])/8.0;
  jacobian[7*3 + 1] =  (1.0 - rst[0])*(1.0 + rst[2])/8.0;
  jacobian[7*3 + 2] =  (1.0 - rst[0])*(1.0 + rst[1])/8.0;
}

// Tetrahedron:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HGRAD_TET_I1_FEMDef.hpp
template<>
void DGHGradBasis<vtkDGCell::Shape::Tetrahedron>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0] = 1.0 - rst[0] - rst[1] - rst[2];
  basis[1] = rst[0];
  basis[2] = rst[1];
  basis[3] = rst[2];
}

template<>
void DGHGradGradient<vtkDGCell::Shape::Tetrahedron>(
  const vtkVector3d& vtkNotUsed(rst), std::vector<double>& jacobian)
{
  jacobian[0*3 + 0] = -1.0;
  jacobian[0*3 + 1] = -1.0;
  jacobian[0*3 + 2] = -1.0;

  jacobian[1*3 + 0] =  1.0;
  jacobian[1*3 + 1] =  0.0;
  jacobian[1*3 + 2] =  0.0;

  jacobian[2*3 + 0] =  0.0;
  jacobian[2*3 + 1] =  1.0;
  jacobian[2*3 + 2] =  0.0;

  jacobian[3*3 + 0] =  0.0;
  jacobian[3*3 + 1] =  0.0;
  jacobian[3*3 + 2] =  1.0;
}

// Wedge:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HGRAD_WEDGE_I1_FEMDef.hpp
template<>
void DGHGradBasis<vtkDGCell::Shape::Wedge>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0] = (1.0 - rst[0] - rst[1])*(1.0 - rst[2])/2.0;
  basis[1] = rst[0]*(1.0 - rst[2])/2.0;
  basis[2] = rst[1]*(1.0 - rst[2])/2.0;
  basis[3] = (1.0 - rst[0] - rst[1])*(1.0 + rst[2])/2.0;
  basis[4] = rst[0]*(1.0 + rst[2])/2.0;
  basis[5] = rst[1]*(1.0 + rst[2])/2.0;
}

template<>
void DGHGradGradient<vtkDGCell::Shape::Wedge>(const vtkVector3d& rst, std::vector<double>& jacobian)
{
  jacobian[0*3 + 0] = -(1.0 - rst[2])/2.0;
  jacobian[0*3 + 1] = -(1.0 - rst[2])/2.0;
  jacobian[0*3 + 2] = -(1.0 - rst[0] - rst[1])/2.0;

  jacobian[1*3 + 0] =  (1.0 - rst[2])/2.0;
  jacobian[1*3 + 1] =  0.0;
  jacobian[1*3 + 2] = -rst[0]/2.0;

  jacobian[2*3 + 0] =  0.0;
  jacobian[2*3 + 1] =  (1.0 - rst[2])/2.0;
  jacobian[2*3 + 2] = -rst[1]/2.0;

  jacobian[3*3 + 0] = -(1.0 + rst[2])/2.0;
  jacobian[3*3 + 1] = -(1.0 + rst[2])/2.0;
  jacobian[3*3 + 2] =  (1.0 - rst[0] - rst[1])/2.0;

  jacobian[4*3 + 0] =  (1.0 + rst[2])/2.0;
  jacobian[4*3 + 1] =  0.0;
  jacobian[4*3 + 2] =  rst[0]/2.0;

  jacobian[5*3 + 0] =  0.0;
  jacobian[5*3 + 1] =  (1.0 + rst[2])/2.0;
  jacobian[5*3 + 2] =  rst[1]/2.0;
}

// Pyramid:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HGRAD_PYR_I1_FEMDef.hpp
template<>
void DGHGradBasis<vtkDGCell::Shape::Pyramid>(const vtkVector3d& rst, std::vector<double>& basis)
{
  constexpr double eps = std::numeric_limits<double>::epsilon();

  double tt = rst[2];
  if (std::abs(tt - 1.0) < eps)
  {
    if (tt <= 1.0) { tt = 1.0 - eps; }
    else { tt = 1.0 + eps; }
  }

  double ttTerm = 0.25/(1.0 - tt);
  basis[0] = (1.0 - rst[0] - tt) * (1.0 - rst[1] - tt) * ttTerm;
  basis[1] = (1.0 + rst[0] - tt) * (1.0 - rst[1] - tt) * ttTerm;
  basis[2] = (1.0 + rst[0] - tt) * (1.0 + rst[1] - tt) * ttTerm;
  basis[3] = (1.0 - rst[0] - tt) * (1.0 + rst[1] - tt) * ttTerm;
  basis[4] = tt;
}

template<>
void DGHGradGradient<vtkDGCell::Shape::Pyramid>(const vtkVector3d& rst, std::vector<double>& jacobian)
{
  constexpr double eps = std::numeric_limits<double>::epsilon();

  double rr = rst[0];
  double ss = rst[1];
  double tt = rst[2];
  // Be sure that the basis functions are defined when tt is very close to 1.
  // Warning: the derivatives are discontinuous in (0, 0, 1).
  if (fabs(tt-1.0) < eps)
  {
    if (tt <= 1.0) { tt = 1.0 - eps; }
    else { tt = 1.0 + eps; }
  }

  double ttTerm = 0.25/(1.0 - tt);
  double ttTerm2 = 4.0 * ttTerm * ttTerm;

  jacobian[0*3 + 0] = (ss + tt - 1.0) * ttTerm;
  jacobian[0*3 + 1] = (rr + tt - 1.0) * ttTerm;
  jacobian[0*3 + 2] = rr * ss * ttTerm2 - 0.25;

  jacobian[1*3 + 0] =  (1.0 - ss - tt) * ttTerm;
  jacobian[1*3 + 1] =  (tt - rr - 1.0) * ttTerm;
  jacobian[1*3 + 2] =  - rr*ss * ttTerm2 - 0.25;

  jacobian[2*3 + 0] =  (1.0 + ss - tt) * ttTerm;
  jacobian[2*3 + 1] =  (1.0 + rr - tt) * ttTerm;
  jacobian[2*3 + 2] =  rr * ss * ttTerm2 - 0.25;

  jacobian[3*3 + 0] =  (tt - ss - 1.0) * ttTerm;
  jacobian[3*3 + 1] =  (1.0 - rr - tt) * ttTerm;
  jacobian[3*3 + 2] =  - rr*ss * ttTerm2 - 0.25;

  jacobian[4*3 + 0] =  0.0;
  jacobian[4*3 + 1] =  0.0;
  jacobian[4*3 + 2] =  1;
}

// Quadrilateral:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HGRAD_QUAD_I1_FEMDef.hpp
template<>
void DGHGradBasis<vtkDGCell::Shape::Quadrilateral>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0] = (1.0 - rst[0])*(1.0 - rst[1])/4.0;
  basis[1] = (1.0 + rst[0])*(1.0 - rst[1])/4.0;
  basis[2] = (1.0 + rst[0])*(1.0 + rst[1])/4.0;
  basis[3] = (1.0 - rst[0])*(1.0 + rst[1])/4.0;
}

template<>
void DGHGradGradient<vtkDGCell::Shape::Quadrilateral>(const vtkVector3d& rst, std::vector<double>& jacobian)
{
  // Jacobian
  jacobian[0*2 + 0] = -(1.0 - rst[1])/4.0;
  jacobian[0*2 + 1] = -(1.0 - rst[0])/4.0;

  jacobian[1*2 + 0] =  (1.0 - rst[1])/4.0;
  jacobian[1*2 + 1] = -(1.0 + rst[0])/4.0;

  jacobian[2*2 + 0] =  (1.0 + rst[1])/4.0;
  jacobian[2*2 + 1] =  (1.0 + rst[0])/4.0;

  jacobian[3*2 + 0] = -(1.0 + rst[1])/4.0;
  jacobian[3*2 + 1] =  (1.0 - rst[0])/4.0;
}

// Triangle:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HGRAD_TRI_I1_FEMDef.hpp
template<>
void DGHGradBasis<vtkDGCell::Shape::Triangle>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0] = 1.0 - rst[0] - rst[1];
  basis[1] = rst[0];
  basis[2] = rst[1];
}

template<>
void DGHGradGradient<vtkDGCell::Shape::Triangle>(
  const vtkVector3d& vtkNotUsed(rst),
  std::vector<double>& jacobian)
{
  jacobian[0*2 + 0] = -1.0;
  jacobian[0*2 + 1] = -1.0;

  jacobian[1*2 + 0] = 1.0;
  jacobian[1*2 + 1] = 0.0;

  jacobian[2*2 + 0] = 0.0;
  jacobian[2*2 + 1] = 1.0;
}

// Line:
// See packages/intrepid/src/Discretization/Basis/Intrepid_HGRAD_LIN_I1_FEMDef.hpp
template<>
void DGHGradBasis<vtkDGCell::Shape::Edge>(const vtkVector3d& rst, std::vector<double>& basis)
{
  basis[0] = (1.0 - rst[0]);
  basis[1] = rst[0];
}

template<>
void DGHGradGradient<vtkDGCell::Shape::Edge>(
  const vtkVector3d& vtkNotUsed(rst), std::vector<double>& jacobian)
{
  jacobian[0] = -1.0;
  jacobian[1] = +1.0;
}

// Vertex:
// Not really supported, but...
template<>
void DGHGradBasis<vtkDGCell::Shape::Vertex>(const vtkVector3d&, std::vector<double>& basis)
{
  basis[0] = 1.0;
}

template<>
void DGHGradGradient<vtkDGCell::Shape::Vertex>(const vtkVector3d&, std::vector<double>&)
{
}

} // anonymous namespace
// clang-format on

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkDGInterpolateCalculator);

void vtkDGInterpolateCalculator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldValues: " << this->FieldValues << "\n";
  os << indent << "ShapeConnectivity: " << this->ShapeConnectivity << "\n";
  os << indent << "ShapeValues: " << this->ShapeValues << "\n";
}

void vtkDGInterpolateCalculator::Evaluate(
  vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& value)
{
  static thread_local std::vector<double> basis;
  bool isVectorBasis;
  // \a basisValueSize is the number of components of each *basis* value (not each coefficient
  // value): For H(grad) spaces, basisValueSize is 1, but H(curl) and H(div) have basisValueSize ==
  // dimension because the basis is a vector-valued function. Note that basisValueSize is **not**
  // the number of basis functions, but rather the number of values produced when any basis function
  // is evaluated.
  int basisValueSize;
  // coeffSize is the number of coefficient values per basis function.
  // Usually, for H(curl) and H(div) function spaces, coeffSize is 1 and basisValueSize is 3.
  // We support coeffSize > 1 but know of no one using it this way.
  std::size_t coeffSize = 1;
  // XXX(c++14)
#if __cplusplus < 201400L
  auto tokenId = this->FunctionSpace.GetId();
  if (tokenId == "DG constant C0"_hash)
  {
    // Constant, discontinuous field over each cell.
    basis.resize(1);
    basis[0] = 1.0;
    isVectorBasis = false;
    basisValueSize = 1;
    coeffSize = this->FieldValues->GetNumberOfComponents();
  }
  else if (tokenId == "CG HGRAD C1"_hash || tokenId == "DG HGRAD C1"_hash)
  {
    // Both CG and DG point basis functions are the same:
    basis.resize(this->NumberOfBasisFunctions);
    vtkDGInvokeBasisForShapes(DGHGradBasis, this->CellShape);
    isVectorBasis = false;
    basisValueSize = 1;
    // For DG attributes, FieldValues contains all the coefficients for all
    // the basis functions in a single tuple per cell.
    // For CG attributes, FieldValues contains one basis-function's worth of
    // coefficients per tuple.
    coeffSize = (this->FunctionSpace.GetId() == "DG HGRAD C1"_hash
        ? this->FieldValues->GetNumberOfComponents() / this->NumberOfBasisFunctions
        : this->FieldValues->GetNumberOfComponents());
  }
  else if (tokenId == "CG HCURL I1"_hash || tokenId == "DG HCURL I1"_hash)
  {
    // Nedelec with no shared edge data (even though sometimes advertised as CG).
    basis.resize(this->Dimension * this->NumberOfBasisFunctions);
    vtkDGInvokeBasisForShapes(CGHCurlBasis, this->CellShape);
    isVectorBasis = true;
    basisValueSize = this->Dimension;
    // TODO: This assumes CG HCURL is stored as if it were a DG attribute.
    //       The convention will change when IOSS is updated to use edge
    //       blocks to hold all coefficients common to an edge.
    coeffSize = this->FieldValues->GetNumberOfComponents() / this->NumberOfBasisFunctions;
  }
  else if (tokenId == "CG HDIV I1"_hash)
  {
    // Raviart-Thomas? with no shared face data (even though advertised as CG).
    basis.resize(this->Dimension * this->NumberOfBasisFunctions);
    vtkDGInvokeBasisForShapes(CGHDivBasis, this->CellShape);
    isVectorBasis = true;
    basisValueSize = this->Dimension;
    // TODO: This assumes CG HDIV is stored as if it were a DG attribute.
    //       The convention will change when IOSS is updated to use face
    //       blocks to hold all coefficients common to a face.
    coeffSize = this->FieldValues->GetNumberOfComponents() / this->NumberOfBasisFunctions;
  }
  else
  {
    vtkErrorMacro("Unsupported function space \"" << this->FunctionSpace.Data() << "\".");
    basisValueSize = 0;
    coeffSize = 0;
    isVectorBasis = false;
    basis.resize(0);
  }
#else
  switch (this->FunctionSpace.GetId())
  {
    case "DG constant C0"_hash: // Constant, discontinuous field over each cell.
      basis.resize(1);
      basis[0] = 1.0;
      isVectorBasis = false;
      basisValueSize = 1;
      coeffSize = this->FieldValues->GetNumberOfComponents();
      break;
      // Both CG and DG point basis functions are the same:
    case "CG HGRAD C1"_hash: // Continuous field; shared connectivity
    case "DG HGRAD C1"_hash: // Lagrange with no shared point connectivity.
      basis.resize(this->NumberOfBasisFunctions);
      vtkDGInvokeBasisForShapes(DGHGradBasis, this->CellShape);
      isVectorBasis = false;
      basisValueSize = 1;
      // For DG attributes, FieldValues contains all the coefficients for all
      // the basis functions in a single tuple per cell.
      // For CG attributes, FieldValues contains one basis-function's worth of
      // coefficients per tuple.
      coeffSize = (this->FunctionSpace.GetId() == "DG HGRAD C1"_hash
          ? this->FieldValues->GetNumberOfComponents() / this->NumberOfBasisFunctions
          : this->FieldValues->GetNumberOfComponents());
      break;
    case "CG HCURL I1"_hash:
    case "DG HCURL I1"_hash:
      // Nedelec with no shared edge data (even though sometimes advertised as CG).
      basis.resize(this->Dimension * this->NumberOfBasisFunctions);
      vtkDGInvokeBasisForShapes(CGHCurlBasis, this->CellShape);
      isVectorBasis = true;
      basisValueSize = this->Dimension;
      // TODO: This assumes CG HCURL is stored as if it were a DG attribute.
      //       The convention will change when IOSS is updated to use edge
      //       blocks to hold all coefficients common to an edge.
      coeffSize = this->FieldValues->GetNumberOfComponents() / this->NumberOfBasisFunctions;
      break;
    case "CG HDIV I1"_hash:
      // Raviart-Thomas? with no shared face data (even though advertised as CG).
      basis.resize(this->Dimension * this->NumberOfBasisFunctions);
      vtkDGInvokeBasisForShapes(CGHDivBasis, this->CellShape);
      isVectorBasis = true;
      basisValueSize = this->Dimension;
      // TODO: This assumes CG HDIV is stored as if it were a DG attribute.
      //       The convention will change when IOSS is updated to use face
      //       blocks to hold all coefficients common to a face.
      coeffSize = this->FieldValues->GetNumberOfComponents() / this->NumberOfBasisFunctions;
      break;
    default:
    {
      vtkErrorMacro("Unsupported function space \"" << this->FunctionSpace.Data() << "\".");
    }
    break;
  }
#endif
  // valueSize is the number of components in a single tuple produced by evaluating the
  // cell-attribute. This will be 1 for scalars, 2 or 3 for vectors, 6 for symmetric tensors, etc.
  // std::size_t valueSize = coeffSize * basisValueSize;
  value.resize(this->FieldValues->GetNumberOfComponents() * basisValueSize);
  for (std::size_t ii = 0; ii < value.size(); ++ii)
  {
    value[ii] = 0.;
  }
  static thread_local std::vector<double> coefficients;
  // coefficients.resize(this->FieldValues->GetNumberOfComponents());
  coefficients.resize(this->NumberOfBasisFunctions * coeffSize);
  std::vector<vtkTypeInt64> conn;
  conn.resize(this->NumberOfBasisFunctions);
  // int nn = static_cast<int>(this->FieldValues->GetNumberOfComponents()) /
  // this->NumberOfBasisFunctions;
  // XXX(c++14)
#if __cplusplus < 201400L
  tokenId = this->FunctionSpace.GetId();
  if (tokenId == "CG HGRAD C1"_hash)
  {
    // Continuous fields share point coordinates via connectivity. Fetch it:
    this->FieldConnectivity->GetTypedTuple(cellId, &conn[0]);
    // Now use connectivity as an indirection when fetching value tuples:
    for (int ii = 0; ii < this->NumberOfBasisFunctions; ++ii)
    {
      this->FieldValues->GetTuple(conn[ii], &coefficients[ii * this->NumberOfComponents]);
      for (std::size_t jj = 0; jj < coeffSize; ++jj)
      {
        value[jj] += coefficients[ii * coeffSize + jj] * basis[ii];
      }
    }
  }
  else
  {
    // Discontinuous fields do not share point coordinates.
    for (int ii = 0; ii < this->NumberOfBasisFunctions; ++ii)
    {
      this->FieldValues->GetTuple(cellId, &coefficients[0]);
      for (int dd = 0; dd < basisValueSize; ++dd)
      {
        for (std::size_t jj = 0; jj < coeffSize; ++jj)
        {
          value[jj * basisValueSize + dd] +=
            coefficients[ii * coeffSize + jj] * basis[ii * basisValueSize + dd];
        }
      }
    }
  }
#else
  switch (this->FunctionSpace.GetId())
  {
    case "CG HGRAD C1"_hash:
      // Continuous fields share point coordinates via connectivity. Fetch it:
      this->FieldConnectivity->GetTypedTuple(cellId, &conn[0]);
      // Now use connectivity as an indirection when fetching value tuples:
      for (int ii = 0; ii < this->NumberOfBasisFunctions; ++ii)
      {
        this->FieldValues->GetTuple(conn[ii], &coefficients[ii * this->NumberOfComponents]);
        for (std::size_t jj = 0; jj < coeffSize; ++jj)
        {
          value[jj] += coefficients[ii * coeffSize + jj] * basis[ii];
        }
      }
      break;
    default:
      // Discontinuous fields do not share point coordinates.
      for (int ii = 0; ii < this->NumberOfBasisFunctions; ++ii)
      {
        this->FieldValues->GetTuple(cellId, &coefficients[0]);
        for (int dd = 0; dd < basisValueSize; ++dd)
        {
          for (std::size_t jj = 0; jj < coeffSize; ++jj)
          {
            value[jj * basisValueSize + dd] +=
              coefficients[ii * coeffSize + jj] * basis[ii * basisValueSize + dd];
          }
        }
      }
      break;
  }
#endif
  // Now, for H(curl) and H(div) elements, transform the resulting vectors
  // by the inverse Jacobian of the cell's *shape* function.
  if (isVectorBasis)
  {
    static thread_local std::vector<double> spatialDeriv;
    this->InternalDerivative<true>(cellId, rst, spatialDeriv, 1e-3);
    // TODO: Complain if NumberOfComponents is not 1?
    std::vector<double> tmp(this->Dimension * this->NumberOfComponents, 0.);
    for (int ii = 0; ii < this->NumberOfComponents; ++ii)
    {
      for (int jj = 0; jj < this->Dimension; ++jj)
      {
        for (int kk = 0; kk < 3; ++kk)
        {
          tmp[ii * this->NumberOfComponents + kk] +=
            spatialDeriv[kk * this->Dimension + jj] * value[ii * basisValueSize + jj];
        }
      }
    }
    // Copy the transformed result into the output variable.
    value = tmp;
  }
}

bool vtkDGInterpolateCalculator::AnalyticDerivative() const
{
  // XXX(c++14)
#if __cplusplus < 201400L
  auto tokenId = this->FunctionSpace.GetId();
  if (tokenId == "CG HGRAD C1"_hash || tokenId == "DG HGRAD C1"_hash)
  {
    return true;
  }
#else
  switch (this->FunctionSpace.GetId())
  {
    case "CG HGRAD C1"_hash:
    case "DG HGRAD C1"_hash:
      return true;
    default:
      break;
  }
#endif
  return false;
}

template <bool UseShape>
void vtkDGInterpolateCalculator::InternalDerivative(
  vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& jacobian, double neighborhood)
{
  // We only support an analytical gradient for the H(Grad) function space.
  // Others are forwarded to the superclass's finite-difference approximation:
  static thread_local std::vector<double>
    fieldCoefficients; // NumberOfComponents * NumberOfBasisFunctions
  fieldCoefficients.resize(UseShape ? 3 * this->NumberOfBasisFunctions
                                    : this->NumberOfComponents * this->NumberOfBasisFunctions);
  // TODO: When evaluating the shape-function derivative, we only
  //       support CG HGRAD C1 basis functions for now. Without
  //       modifying the switch statement, we end up recursing until
  //       the stack is exhausted for HCURL/HDIV cell-attributes.
  //       This is because these bases need the Jacobian of the
  //       *shape* attribute but curl/div spaces don't provide an
  //       analytic derivative. Instead, a finite difference approximation
  //       is made. But that requires evaluating the function which
  //       in turn needs another Jacobian evaluation (and so on).
  // XXX(c++14)
#if __cplusplus < 201400L
  auto tokenId = UseShape ? "CG HGRAD C1"_hash : this->FunctionSpace.GetId();
  if (tokenId == "CG HGRAD C1"_hash)
  {
    // Fetch and copy shared coefficients into fieldCoefficients.
    {
      static thread_local std::vector<vtkTypeInt64> conn;
      conn.resize(this->NumberOfBasisFunctions);
      if (UseShape)
      {
        this->ShapeConnectivity->GetTypedTuple(cellId, &conn[0]);
      }
      else
      {
        this->FieldConnectivity->GetTypedTuple(cellId, &conn[0]);
      }
      for (int ii = 0; ii < this->NumberOfBasisFunctions; ++ii)
      {
        if (UseShape)
        {
          this->ShapeValues->GetTuple(conn[ii], &fieldCoefficients[ii * this->NumberOfComponents]);
        }
        else
        {
          this->FieldValues->GetTuple(conn[ii], &fieldCoefficients[ii * this->NumberOfComponents]);
        }
      }
    }
  }
  else if (tokenId == "DG HGRAD C1"_hash)
  {
    // Fetch the cell's coefficients as a single row.
    if (UseShape)
    {
      this->ShapeValues->GetTuple(cellId, &fieldCoefficients[0]);
    }
    else
    {
      this->FieldValues->GetTuple(cellId, &fieldCoefficients[0]);
    }
  }
  else
  {
    this->Superclass::EvaluateDerivative(cellId, rst, jacobian, neighborhood);
    return;
  }
#else
  switch (UseShape ? "CG HGRAD C1"_hash : this->FunctionSpace.GetId())
  {
    case "CG HGRAD C1"_hash:
      // Fetch and copy shared coefficients into fieldCoefficients.
      {
        static thread_local std::vector<vtkTypeInt64> conn;
        conn.resize(this->NumberOfBasisFunctions);
        if (UseShape)
        {
          this->ShapeConnectivity->GetTypedTuple(cellId, &conn[0]);
        }
        else
        {
          this->FieldConnectivity->GetTypedTuple(cellId, &conn[0]);
        }
        for (int ii = 0; ii < this->NumberOfBasisFunctions; ++ii)
        {
          if (UseShape)
          {
            this->ShapeValues->GetTuple(
              conn[ii], &fieldCoefficients[ii * this->NumberOfComponents]);
          }
          else
          {
            this->FieldValues->GetTuple(
              conn[ii], &fieldCoefficients[ii * this->NumberOfComponents]);
          }
        }
      }
      break;
    case "DG HGRAD C1"_hash:
      // Fetch the cell's coefficients as a single row.
      if (UseShape)
      {
        this->ShapeValues->GetTuple(cellId, &fieldCoefficients[0]);
      }
      else
      {
        this->FieldValues->GetTuple(cellId, &fieldCoefficients[0]);
      }
      break;
    default:
      this->Superclass::EvaluateDerivative(cellId, rst, jacobian, neighborhood);
      return;
  }
#endif
  static thread_local std::vector<double> basis;
  basis.resize(this->NumberOfBasisFunctions * this->Dimension);
  vtkDGInvokeBasisForShapes(DGHGradGradient, this->CellShape);
  jacobian.resize(UseShape ? 3 * this->Dimension : this->NumberOfComponents * this->Dimension);
  // Fill the jacobian with zeros so we can accumulate into it:
  for (auto& entry : jacobian)
  {
    entry = 0.0;
  }
  // Iterate over coefficient data, accumulating it weighted by the gradient of the basis functions.
  for (int ii = 0; ii < this->NumberOfBasisFunctions; ++ii)
  {
    for (int jj = 0; jj < (UseShape ? 3 : this->NumberOfComponents); ++jj)
    {
      for (int dd = 0; dd < this->Dimension; ++dd)
      {
        jacobian[jj * this->Dimension + dd] +=
          basis[ii * this->Dimension + dd] * fieldCoefficients[ii * this->NumberOfComponents + jj];
      }
    }
  }
}

void vtkDGInterpolateCalculator::EvaluateDerivative(
  vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& jacobian, double neighborhood)
{
  this->InternalDerivative<false>(cellId, rst, jacobian, neighborhood);
}

vtkSmartPointer<vtkCellAttributeCalculator> vtkDGInterpolateCalculator::PrepareForGrid(
  vtkCellMetadata* cell, vtkCellAttribute* field)
{
  auto* dgCell = vtkDGCell::SafeDownCast(cell);
  if (!dgCell)
  {
    return nullptr;
  }

  // Clone ourselves for this new context.
  vtkNew<vtkDGInterpolateCalculator> result;

  auto* grid = cell->GetCellGrid();
  auto* shape = grid->GetShapeAttribute();
  vtkStringToken shapeTag = shape->GetAttributeType();
  if (shapeTag != "CG HGRAD C1"_token)
  {
    vtkErrorMacro("Unsupported cell shape function space \"" << shapeTag.Data() << "\".");
    return nullptr;
  }

  vtkStringToken fieldTag = field->GetAttributeType();
  result->FunctionSpace = fieldTag;
  // XXX(c++14)
#if __cplusplus < 201400L
  auto tokenId = fieldTag.GetId();
  if (tokenId == "CG HGRAD C1"_hash || tokenId == "DG HGRAD C1"_hash)
  {
    result->NumberOfBasisFunctions = dgCell->GetNumberOfSidesOfDimension(0);
  }
  else if (tokenId == "CG HCURL I1"_hash || tokenId == "DG HCURL I1"_hash)
  {
    result->NumberOfBasisFunctions = dgCell->GetNumberOfSidesOfDimension(1);
  }
  else if (tokenId == "CG HDIV I1"_hash)
  {
    // For 2-D cells (quad, tri) a "face" is an edge.
    result->NumberOfBasisFunctions =
      dgCell->GetNumberOfSidesOfDimension(dgCell->GetDimension() == 3 ? 2 : 1);
  }
  else if (tokenId == "DG constant C0"_hash)
  {
    result->NumberOfBasisFunctions = 1;
  }
  else
  {
    vtkErrorMacro("Unsupported field type \"" << fieldTag.Data() << "\".");
    return nullptr;
  }
#else
  switch (fieldTag.GetId())
  {
    case "DG constant C0"_hash:
      result->NumberOfBasisFunctions = 1;
      break;
    case "CG HGRAD C1"_hash:
    case "DG HGRAD C1"_hash:
      result->NumberOfBasisFunctions = dgCell->GetNumberOfSidesOfDimension(0);
      break;
    case "CG HCURL I1"_hash:
    case "DG HCURL I1"_hash:
      result->NumberOfBasisFunctions = dgCell->GetNumberOfSidesOfDimension(1);
      break;
    case "CG HDIV I1"_hash:
      // For 2-D cells (quad, tri) a "face" is an edge.
      result->NumberOfBasisFunctions =
        dgCell->GetNumberOfSidesOfDimension(dgCell->GetDimension() == 3 ? 2 : 1);
      break;
    default:
    {
      vtkErrorMacro("Unsupported field type \"" << fieldTag.Data() << "\".");
      return nullptr;
    }
  }
#endif

  result->Dimension = dgCell->GetDimension();
  result->CellShape = dgCell->GetShape();
  result->NumberOfComponents = field->GetNumberOfComponents();

  auto shapeArrays = shape->GetArraysForCellType(dgCell->GetClassName());
  result->ShapeValues = vtkDataArray::SafeDownCast(shapeArrays["values"_token]);
  result->ShapeConnectivity = vtkTypeInt64Array::SafeDownCast(shapeArrays["connectivity"_token]);

  auto fieldArrays = field->GetArraysForCellType(dgCell->GetClassName());
  result->FieldValues = vtkDataArray::SafeDownCast(fieldArrays["values"_token]);
  result->FieldConnectivity = vtkTypeInt64Array::SafeDownCast(fieldArrays["connectivity"_token]);

  return result;
}

VTK_ABI_NAMESPACE_END
