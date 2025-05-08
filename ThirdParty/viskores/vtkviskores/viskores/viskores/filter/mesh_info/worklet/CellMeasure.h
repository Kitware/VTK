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

#ifndef viskores_worklet_CellMeasure_h
#define viskores_worklet_CellMeasure_h

#include <viskores/exec/CellMeasure.h>
#include <viskores/filter/mesh_info/CellMeasures.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{

namespace worklet
{

/**\brief Simple functor that returns the spatial integral of each cell as a cell field.
  *
  * The integration is done over the spatial extent of the cell and thus units
  * are either null, arc length, area, or volume depending on whether the parametric
  * dimension of the cell is 0 (vertices), 1 (curves), 2 (surfaces), or 3 (volumes).
  * The template parameter of this class configures which types of cells (based on their
  * parametric dimensions) should be integrated. Other cells will report 0.
  *
  * Note that the integrals are signed; inverted cells will report negative values.
  */
class CellMeasure : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint pointCoords,
                                FieldOutCell volumesOut);
  using ExecutionSignature = void(CellShape, PointCount, _2, _3);
  using InputDomain = _1;

  explicit CellMeasure(viskores::filter::mesh_info::IntegrationType m)
    : measure(m)
  {
  }

  template <typename CellShape, typename PointCoordVecType, typename OutType>
  VISKORES_EXEC void operator()(CellShape shape,
                                const viskores::IdComponent& numPoints,
                                const PointCoordVecType& pts,
                                OutType& volume) const
  {
    switch (shape.Id)
    {
      viskoresGenericCellShapeMacro(
        volume = this->ComputeMeasure<OutType>(numPoints, pts, CellShapeTag()));
      default:
        this->RaiseError("Asked for volume of unknown cell shape.");
        volume = OutType(0.0);
    }
  }

private:
  template <typename OutType, typename PointCoordVecType, typename CellShapeType>
  VISKORES_EXEC OutType ComputeMeasure(const viskores::IdComponent& numPts,
                                       const PointCoordVecType& pts,
                                       CellShapeType) const
  {
#if defined(VISKORES_MSVC)
#pragma warning(push)
#pragma warning(disable : 4068) //unknown pragma
#endif
#ifdef __NVCC__

#pragma push
#if (CUDART_VERSION >= 11050)
#pragma nv_diag_suppress = code_is_unreachable
#else
#pragma diag_suppress = code_is_unreachable
#endif
#endif
    using viskores::filter::mesh_info::IntegrationType;

    viskores::ErrorCode ec;
    switch (viskores::CellTraits<CellShapeType>::TOPOLOGICAL_DIMENSIONS)
    {
      case 0:
        // Fall through to return 0 measure.
        break;
      case 1:
        if ((this->measure & IntegrationType::ArcLength) == IntegrationType::ArcLength)
        {
          return viskores::exec::CellMeasure<OutType>(numPts, pts, CellShapeType(), ec);
        }
        break;
      case 2:
        if ((this->measure & IntegrationType::Area) == IntegrationType::Area)
        {
          return viskores::exec::CellMeasure<OutType>(numPts, pts, CellShapeType(), ec);
        }
        break;
      case 3:
        if ((this->measure & IntegrationType::Volume) == IntegrationType::Volume)
        {
          return viskores::exec::CellMeasure<OutType>(numPts, pts, CellShapeType(), ec);
        }
        break;
      default:
        // Fall through to return 0 measure.
        break;
    }
    return OutType(0.0);
#ifdef __NVCC__
#pragma pop
#endif
#if defined(VISKORES_MSVC)
#pragma warning(pop)
#endif
  }

  viskores::filter::mesh_info::IntegrationType measure;
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_CellMeasure_h
