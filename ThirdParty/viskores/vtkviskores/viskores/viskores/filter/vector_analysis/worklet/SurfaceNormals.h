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
#ifndef viskores_worklet_SurfaceNormals_h
#define viskores_worklet_SurfaceNormals_h


#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/CellTraits.h>
#include <viskores/TypeTraits.h>
#include <viskores/VectorAnalysis.h>
#include <viskores/cont/UncertainArrayHandle.h>

namespace viskores
{
namespace worklet
{

namespace detail
{
struct PassThrough
{
  template <typename T>
  VISKORES_EXEC viskores::Vec<T, 3> operator()(const viskores::Vec<T, 3>& in) const
  {
    return in;
  }
};

struct Normal
{
  template <typename T>
  VISKORES_EXEC viskores::Vec<T, 3> operator()(const viskores::Vec<T, 3>& in) const
  {
    return viskores::Normal(in);
  }
};
} // detail

class FacetedSurfaceNormals
{
public:
  template <typename NormalFnctr = detail::Normal>
  class Worklet : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset, FieldInPoint points, FieldOutCell normals);
    using ExecutionSignature = void(CellShape, _2, _3);

    using InputDomain = _1;

    template <typename CellShapeTag, typename PointsVecType, typename T>
    VISKORES_EXEC void operator()(CellShapeTag,
                                  const PointsVecType& points,
                                  viskores::Vec<T, 3>& normal) const
    {
      using CTraits = viskores::CellTraits<CellShapeTag>;
      const auto tag = typename CTraits::TopologicalDimensionsTag();
      this->Compute(tag, points, normal);
    }

    template <viskores::IdComponent Dim, typename PointsVecType, typename T>
    VISKORES_EXEC void Compute(viskores::CellTopologicalDimensionsTag<Dim>,
                               const PointsVecType&,
                               viskores::Vec<T, 3>& normal) const
    {
      normal = viskores::TypeTraits<viskores::Vec<T, 3>>::ZeroInitialization();
    }

    template <typename PointsVecType, typename T>
    VISKORES_EXEC void Compute(viskores::CellTopologicalDimensionsTag<2>,
                               const PointsVecType& points,
                               viskores::Vec<T, 3>& normal) const
    {
      normal = this->Normal(viskores::Cross(points[2] - points[1], points[0] - points[1]));
    }



    template <typename PointsVecType, typename T>
    VISKORES_EXEC void operator()(viskores::CellShapeTagGeneric shape,
                                  const PointsVecType& points,
                                  viskores::Vec<T, 3>& normal) const
    {
      switch (shape.Id)
      {
        viskoresGenericCellShapeMacro(this->operator()(CellShapeTag(), points, normal));
        default:
          this->RaiseError("unknown cell type");
          break;
      }
    }

  private:
    NormalFnctr Normal;
  };

  FacetedSurfaceNormals()
    : Normalize(true)
  {
  }

  /// Set/Get if the results should be normalized
  void SetNormalize(bool value) { this->Normalize = value; }
  bool GetNormalize() const { return this->Normalize; }

  template <typename CellSetType, typename PointsType, typename NormalCompType>
  void Run(const CellSetType& cellset,
           const PointsType& points,
           viskores::cont::ArrayHandle<viskores::Vec<NormalCompType, 3>>& normals)
  {
    if (this->Normalize)
    {
      viskores::worklet::DispatcherMapTopology<Worklet<>>().Invoke(cellset, points, normals);
    }
    else
    {
      viskores::worklet::DispatcherMapTopology<Worklet<detail::PassThrough>>().Invoke(
        cellset, points, normals);
    }
  }

private:
  bool Normalize;
};

class SmoothSurfaceNormals
{
public:
  class Worklet : public viskores::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInCell faceNormals,
                                  FieldOutPoint pointNormals);
    using ExecutionSignature = void(CellCount, _2, _3);

    using InputDomain = _1;

    template <typename FaceNormalsVecType, typename T>
    VISKORES_EXEC void operator()(viskores::IdComponent numCells,
                                  const FaceNormalsVecType& faceNormals,
                                  viskores::Vec<T, 3>& pointNormal) const
    {
      if (numCells == 0)
      {
        pointNormal = viskores::TypeTraits<viskores::Vec<T, 3>>::ZeroInitialization();
      }
      else
      {
        auto result = faceNormals[0];
        for (viskores::IdComponent i = 1; i < numCells; ++i)
        {
          result += faceNormals[i];
        }
        pointNormal = viskores::Normal(result);
      }
    }
  };

  template <typename CellSetType, typename NormalCompType, typename FaceNormalStorageType>
  void Run(const CellSetType& cellset,
           const viskores::cont::ArrayHandle<viskores::Vec<NormalCompType, 3>,
                                             FaceNormalStorageType>& faceNormals,
           viskores::cont::ArrayHandle<viskores::Vec<NormalCompType, 3>>& pointNormals)
  {
    viskores::worklet::DispatcherMapTopology<Worklet>().Invoke(cellset, faceNormals, pointNormals);
  }
};
}
} // viskores::worklet

#endif // viskores_worklet_SurfaceNormals_h
