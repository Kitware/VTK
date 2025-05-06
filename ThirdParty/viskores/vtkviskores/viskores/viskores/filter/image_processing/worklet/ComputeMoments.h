//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=======================================================================
#ifndef viskores_worklet_moments_ComputeMoments_h
#define viskores_worklet_moments_ComputeMoments_h

#include <viskores/Math.h>
#include <viskores/worklet/WorkletPointNeighborhood.h>

#include <viskores/cont/ArrayHandleRecombineVec.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/UncertainArrayHandle.h>
#include <viskores/cont/UncertainCellSet.h>

#include <viskores/exec/BoundaryState.h>

#include <cassert>
#include <string>

namespace viskores
{
namespace worklet
{
namespace moments
{

struct ComputeMoments2D : public viskores::worklet::WorkletPointNeighborhood
{
public:
  ComputeMoments2D(const viskores::Vec3f& _spacing, viskores::Float64 _radius, int _p, int _q)
    : RadiusDiscrete(viskores::IdComponent(_radius / (_spacing[0] - 1e-10)),
                     viskores::IdComponent(_radius / (_spacing[1] - 1e-10)),
                     viskores::IdComponent(_radius / (_spacing[2] - 1e-10)))
    , SpacingProduct(_spacing[0] * _spacing[1])
    , p(_p)
    , q(_q)
  {
    assert(_spacing[0] > 1e-10);
    assert(_spacing[1] > 1e-10);
    assert(_spacing[2] > 1e-10);

    assert(_p >= 0);
    assert(_q >= 0);
  }

  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);

  using ExecutionSignature = void(_2, Boundary, _3);

  template <typename NeighIn, typename TOut>
  VISKORES_EXEC void operator()(const NeighIn& image,
                                const viskores::exec::BoundaryState& boundary,
                                TOut& moment) const
  {
    using ComponentType = typename TOut::ComponentType;
    const viskores::IdComponent numComponents = moment.GetNumberOfComponents();

    // For variable sized Vecs, need to iterate over each component.
    for (viskores::IdComponent componentI = 0; componentI < numComponents; ++componentI)
    {
      moment[componentI] = viskores::TypeTraits<ComponentType>::ZeroInitialization();
    }

    // Clamp the radius to the dataset bounds (discard out-of-bounds points).
    const auto minRadius = boundary.ClampNeighborIndex(-this->RadiusDiscrete);
    const auto maxRadius = boundary.ClampNeighborIndex(this->RadiusDiscrete);

    viskores::Vec2f_64 radius;
    for (viskores::IdComponent j = minRadius[1]; j <= maxRadius[1]; ++j)
    {
      if (j > -this->RadiusDiscrete[1] && boundary.IJK[1] + j == 0)
      { // Don't double count samples that exist on other nodes:
        continue;
      }
      radius[1] = j * 1. / this->RadiusDiscrete[1];

      for (viskores::IdComponent i = minRadius[0]; i <= maxRadius[0]; ++i)
      {
        if (i > -this->RadiusDiscrete[0] && boundary.IJK[0] + i == 0)
        { // Don't double count samples that exist on other nodes:
          continue;
        }
        radius[0] = i * 1. / this->RadiusDiscrete[0];

        if (viskores::Dot(radius, radius) <= 1)
        {
          ComponentType multiplier =
            static_cast<ComponentType>(viskores::Pow(radius[0], p) * viskores::Pow(radius[1], q));
          auto inputField = image.Get(i, j, 0);
          // For variable sized Vecs, need to iterate over each component.
          for (viskores::IdComponent componentI = 0; componentI < numComponents; ++componentI)
          {
            moment[componentI] += multiplier * inputField[componentI];
          }
        }
      }
    }

    // For variable sized Vecs, need to iterate over each component.
    for (viskores::IdComponent componentI = 0; componentI < numComponents; ++componentI)
    {
      moment[componentI] *= static_cast<ComponentType>(this->SpacingProduct);
    }
  }

private:
  viskores::Vec3i_32 RadiusDiscrete;
  const viskores::Float64 SpacingProduct;
  const int p;
  const int q;
};

struct ComputeMoments3D : public viskores::worklet::WorkletPointNeighborhood
{
public:
  ComputeMoments3D(const viskores::Vec3f& _spacing,
                   viskores::Float64 _radius,
                   int _p,
                   int _q,
                   int _r)
    : RadiusDiscrete(viskores::IdComponent(_radius / (_spacing[0] - 1e-10)),
                     viskores::IdComponent(_radius / (_spacing[1] - 1e-10)),
                     viskores::IdComponent(_radius / (_spacing[2] - 1e-10)))
    , SpacingProduct(viskores::ReduceProduct(_spacing))
    , p(_p)
    , q(_q)
    , r(_r)
  {
    assert(_spacing[0] > 1e-10);
    assert(_spacing[1] > 1e-10);
    assert(_spacing[2] > 1e-10);

    assert(_p >= 0);
    assert(_q >= 0);
    assert(_r >= 0);
  }

  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);

  using ExecutionSignature = void(_2, Boundary, _3);

  template <typename NeighIn, typename TOut>
  VISKORES_EXEC void operator()(const NeighIn& image,
                                const viskores::exec::BoundaryState& boundary,
                                TOut& moment) const
  {
    using ComponentType = typename TOut::ComponentType;
    const viskores::IdComponent numComponents = moment.GetNumberOfComponents();

    // For variable sized Vecs, need to iterate over each component.
    for (viskores::IdComponent componentI = 0; componentI < numComponents; ++componentI)
    {
      moment[componentI] = viskores::TypeTraits<ComponentType>::ZeroInitialization();
    }

    // Clamp the radius to the dataset bounds (discard out-of-bounds points).
    const auto minRadius = boundary.ClampNeighborIndex(-this->RadiusDiscrete);
    const auto maxRadius = boundary.ClampNeighborIndex(this->RadiusDiscrete);

    viskores::Vec3f_64 radius;
    for (viskores::IdComponent k = minRadius[2]; k <= maxRadius[2]; ++k)
    {
      if (k > -this->RadiusDiscrete[2] && boundary.IJK[2] + k == 0)
      { // Don't double count samples that exist on other nodes:
        continue;
      }
      radius[2] = k * 1. / this->RadiusDiscrete[2];

      for (viskores::IdComponent j = minRadius[1]; j <= maxRadius[1]; ++j)
      {
        if (j > -this->RadiusDiscrete[1] && boundary.IJK[1] + j == 0)
        { // Don't double count samples that exist on other nodes:
          continue;
        }
        radius[1] = j * 1. / this->RadiusDiscrete[1];

        for (viskores::IdComponent i = minRadius[0]; i <= maxRadius[0]; ++i)
        {
          if (i > -this->RadiusDiscrete[0] && boundary.IJK[0] + i == 0)
          { // Don't double count samples that exist on other nodes:
            continue;
          }
          radius[0] = i * 1. / this->RadiusDiscrete[0];

          if (viskores::Dot(radius, radius) <= 1)
          {
            ComponentType multiplier =
              static_cast<ComponentType>(viskores::Pow(radius[0], p) * viskores::Pow(radius[1], q) *
                                         viskores::Pow(radius[2], r));
            auto inputField = image.Get(i, j, k);
            // For variable sized Vecs, need to iterate over each component.
            for (viskores::IdComponent componentI = 0; componentI < numComponents; ++componentI)
            {
              moment[componentI] += multiplier * inputField[componentI];
            }
          }
        }
      }
    }

    // For variable sized Vecs, need to iterate over each component.
    for (viskores::IdComponent componentI = 0; componentI < numComponents; ++componentI)
    {
      moment[componentI] *= static_cast<ComponentType>(this->SpacingProduct);
    }
  }

private:
  viskores::Vec3i_32 RadiusDiscrete;
  const viskores::Float64 SpacingProduct;
  const int p;
  const int q;
  const int r;
};

class ComputeMoments
{
public:
  ComputeMoments(double _radius, const viskores::Vec3f& _spacing)
    : Radius(_radius)
    , Spacing(_spacing)
  {
  }

  class ResolveUnknownCellSet
  {
  public:
    template <typename T>
    void operator()(const viskores::cont::CellSetStructured<2>& input,
                    const viskores::cont::ArrayHandleRecombineVec<T>& pixels,
                    viskores::Vec3f spacing,
                    viskores::Float64 radius,
                    int maxOrder,
                    viskores::cont::DataSet& output) const
    {
      using WorkletType = viskores::worklet::moments::ComputeMoments2D;
      using DispatcherType = viskores::worklet::DispatcherPointNeighborhood<WorkletType>;

      for (int order = 0; order <= maxOrder; ++order)
      {
        for (int p = 0; p <= order; ++p)
        {
          const int q = order - p;

          viskores::cont::ArrayHandleRuntimeVec<T> moments{ pixels.GetNumberOfComponents() };

          DispatcherType dispatcher(WorkletType{ spacing, radius, p, q });
          dispatcher.Invoke(input, pixels, moments);

          std::string fieldName = std::string("index") + std::string(p, '0') + std::string(q, '1');

          viskores::cont::Field momentsField(
            fieldName, viskores::cont::Field::Association::Points, moments);
          output.AddField(momentsField);
        }
      }
    }

    template <typename T>
    void operator()(const viskores::cont::CellSetStructured<3>& input,
                    const viskores::cont::ArrayHandleRecombineVec<T>& pixels,
                    viskores::Vec3f spacing,
                    viskores::Float64 radius,
                    int maxOrder,
                    viskores::cont::DataSet& output) const
    {
      using WorkletType = viskores::worklet::moments::ComputeMoments3D;
      using DispatcherType = viskores::worklet::DispatcherPointNeighborhood<WorkletType>;

      for (int order = 0; order <= maxOrder; ++order)
      {
        for (int r = 0; r <= order; ++r)
        {
          const int qMax = order - r;
          for (int q = 0; q <= qMax; ++q)
          {
            const int p = order - r - q;

            viskores::cont::ArrayHandleRuntimeVec<T> moments{ pixels.GetNumberOfComponents() };

            DispatcherType dispatcher(WorkletType{ spacing, radius, p, q, r });
            dispatcher.Invoke(input, pixels, moments);

            std::string fieldName = std::string("index") + std::string(p, '0') +
              std::string(q, '1') + std::string(r, '2');

            viskores::cont::Field momentsField(
              fieldName, viskores::cont::Field::Association::Points, moments);
            output.AddField(momentsField);
          }
        }
      }
    }
  };

  template <typename T>
  void Run(const viskores::cont::UnknownCellSet& input,
           const viskores::cont::ArrayHandleRecombineVec<T>& pixels,
           int maxOrder,
           viskores::cont::DataSet& output) const
  {
    input.ResetCellSetList(viskores::cont::CellSetListStructured())
      .CastAndCall(ResolveUnknownCellSet(), pixels, this->Spacing, this->Radius, maxOrder, output);
  }

private:
  const viskores::Float64 Radius;
  const viskores::Vec3f Spacing;
};
}
}
}

#endif // viskores_worklet_moments_ComputeMoments_h
