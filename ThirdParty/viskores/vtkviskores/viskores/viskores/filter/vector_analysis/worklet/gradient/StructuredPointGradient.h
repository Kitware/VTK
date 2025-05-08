//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_worklet_gradient_StructuredPointGradient_h
#define viskores_worklet_gradient_StructuredPointGradient_h

#include <viskores/filter/vector_analysis/worklet/gradient/GradientOutput.h>
#include <viskores/worklet/WorkletPointNeighborhood.h>


namespace viskores
{
namespace worklet
{
namespace gradient
{

struct StructuredPointGradient : public viskores::worklet::WorkletPointNeighborhood
{

  using ControlSignature = void(CellSetIn,
                                FieldInNeighborhood points,
                                FieldInNeighborhood,
                                GradientOutputs outputFields);

  using ExecutionSignature = void(Boundary, _2, _3, _4);

  using InputDomain = _1;

  template <typename PointsIn, typename FieldIn, typename GradientOutType>
  VISKORES_EXEC void operator()(const viskores::exec::BoundaryState& boundary,
                                const PointsIn& inputPoints,
                                const FieldIn& inputField,
                                GradientOutType& outputGradient) const
  {
    using CoordType = typename PointsIn::ValueType;
    using CT = typename viskores::VecTraits<CoordType>::BaseComponentType;
    using OT = typename GradientOutType::ComponentType;

    viskores::Vec<CT, 3> xi, eta, zeta;
    viskores::Vec<bool, 3> onBoundary{ !boundary.IsRadiusInXBoundary(1),
                                       !boundary.IsRadiusInYBoundary(1),
                                       !boundary.IsRadiusInZBoundary(1) };

    this->Jacobian(inputPoints, onBoundary, xi, eta, zeta); //store the metrics in xi,eta,zeta

    auto dxi = inputField.Get(1, 0, 0) - inputField.Get(-1, 0, 0);
    auto deta = inputField.Get(0, 1, 0) - inputField.Get(0, -1, 0);
    auto dzeta = inputField.Get(0, 0, 1) - inputField.Get(0, 0, -1);

    dxi = (onBoundary[0] ? dxi : dxi * 0.5f);
    deta = (onBoundary[1] ? deta : deta * 0.5f);
    dzeta = (onBoundary[2] ? dzeta : dzeta * 0.5f);

    outputGradient[0] = static_cast<OT>(xi[0] * dxi + eta[0] * deta + zeta[0] * dzeta);
    outputGradient[1] = static_cast<OT>(xi[1] * dxi + eta[1] * deta + zeta[1] * dzeta);
    outputGradient[2] = static_cast<OT>(xi[2] * dxi + eta[2] * deta + zeta[2] * dzeta);
  }

  template <typename FieldIn, typename GradientOutType>
  VISKORES_EXEC void operator()(
    const viskores::exec::BoundaryState& boundary,
    const viskores::exec::FieldNeighborhood<viskores::internal::ArrayPortalUniformPointCoordinates>&
      inputPoints,
    const FieldIn& inputField,
    GradientOutType& outputGradient) const
  {
    //When the points and cells are both structured we can achieve even better
    //performance by not doing the Jacobian, but instead do an image gradient
    //using central differences
    using PointsIn =
      viskores::exec::FieldNeighborhood<viskores::internal::ArrayPortalUniformPointCoordinates>;
    using CoordType = typename PointsIn::ValueType;
    using OT = typename GradientOutType::ComponentType;

    CoordType r = inputPoints.Portal.GetSpacing();

#if (defined(VISKORES_CUDA) && defined(VISKORES_GCC))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
    if (boundary.IsRadiusInXBoundary(1))
    {
      auto dx = inputField.GetUnchecked(1, 0, 0) - inputField.GetUnchecked(-1, 0, 0);
      outputGradient[0] = static_cast<OT>((dx * 0.5f) / r[0]);
    }
    else
    {
      auto dx = inputField.Get(1, 0, 0) - inputField.Get(-1, 0, 0);
      outputGradient[0] = static_cast<OT>(dx / r[0]);
    }

    if (boundary.IsRadiusInYBoundary(1))
    {
      auto dy = inputField.GetUnchecked(0, 1, 0) - inputField.GetUnchecked(0, -1, 0);
      outputGradient[1] = static_cast<OT>((dy * 0.5f) / r[1]);
    }
    else
    {
      auto dy = inputField.Get(0, 1, 0) - inputField.Get(0, -1, 0);
      outputGradient[1] = static_cast<OT>(dy / (r[1]));
    }

    if (boundary.IsRadiusInZBoundary(1))
    {
      auto dz = inputField.GetUnchecked(0, 0, 1) - inputField.GetUnchecked(0, 0, -1);
      outputGradient[2] = static_cast<OT>((dz * 0.5f) / r[2]);
    }
    else
    {
      auto dz = inputField.Get(0, 0, 1) - inputField.Get(0, 0, -1);
      outputGradient[2] = static_cast<OT>(dz / (r[2]));
    }
#if (defined(VISKORES_CUDA) && defined(VISKORES_GCC))
#pragma GCC diagnostic pop
#endif
  }

  //we need to pass the coordinates into this function, and instead
  //of the input being Vec<coordtype,3> it needs to be Vec<float,3> as the metrics
  //will be float,3 even when T is a 3 component field
  template <typename PointsIn, typename CT>
  VISKORES_EXEC void Jacobian(const PointsIn& inputPoints,
                              const viskores::Vec<bool, 3>& onBoundary,
                              viskores::Vec<CT, 3>& m_xi,
                              viskores::Vec<CT, 3>& m_eta,
                              viskores::Vec<CT, 3>& m_zeta) const
  {
    using CoordType = typename PointsIn::ValueType;
    CoordType xi, eta, zeta;


    if (onBoundary[0])
    {
      xi = (inputPoints.Get(1, 0, 0) - inputPoints.Get(-1, 0, 0));
    }
    else
    {
      xi = (inputPoints.GetUnchecked(1, 0, 0) - inputPoints.GetUnchecked(-1, 0, 0)) * 0.5f;
    }

    if (onBoundary[1])
    {
      eta = (inputPoints.Get(0, 1, 0) - inputPoints.Get(0, -1, 0));
    }
    else
    {
      eta = (inputPoints.GetUnchecked(0, 1, 0) - inputPoints.GetUnchecked(0, -1, 0)) * 0.5f;
    }

    if (onBoundary[2])
    {
      zeta = (inputPoints.Get(0, 0, 1) - inputPoints.Get(0, 0, -1));
    }
    else
    {
      zeta = (inputPoints.GetUnchecked(0, 0, 1) - inputPoints.GetUnchecked(0, 0, -1)) * 0.5f;
    }

    CT aj = xi[0] * eta[1] * zeta[2] + xi[1] * eta[2] * zeta[0] + xi[2] * eta[0] * zeta[1] -
      xi[2] * eta[1] * zeta[0] - xi[1] * eta[0] * zeta[2] - xi[0] * eta[2] * zeta[1];

    aj = (aj != 0.0) ? 1.f / aj : aj;

    //  Xi metrics.
    m_xi[0] = aj * (eta[1] * zeta[2] - eta[2] * zeta[1]);
    m_xi[1] = -aj * (eta[0] * zeta[2] - eta[2] * zeta[0]);
    m_xi[2] = aj * (eta[0] * zeta[1] - eta[1] * zeta[0]);

    //  Eta metrics.
    m_eta[0] = -aj * (xi[1] * zeta[2] - xi[2] * zeta[1]);
    m_eta[1] = aj * (xi[0] * zeta[2] - xi[2] * zeta[0]);
    m_eta[2] = -aj * (xi[0] * zeta[1] - xi[1] * zeta[0]);

    //  Zeta metrics.
    m_zeta[0] = aj * (xi[1] * eta[2] - xi[2] * eta[1]);
    m_zeta[1] = -aj * (xi[0] * eta[2] - xi[2] * eta[0]);
    m_zeta[2] = aj * (xi[0] * eta[1] - xi[1] * eta[0]);
  }
};
}
}
}

#endif
