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

#include <viskores/cont/CellLocatorUniformGrid.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/filter/density_estimate/ParticleDensityCloudInCell.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
class CICWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn coords,
                                FieldIn field,
                                ExecObject locator,
                                WholeCellSetIn<Cell, Point> cellSet,
                                AtomicArrayInOut density);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  template <typename Point,
            typename T,
            typename CellLocatorExecObj,
            typename CellSet,
            typename AtomicArray>
  VISKORES_EXEC void operator()(const Point& point,
                                const T value,
                                const CellLocatorExecObj& locator,
                                const CellSet& cellSet,
                                AtomicArray& density) const
  {
    viskores::Id cellId{};
    viskores::Vec3f parametric;

    if (locator.FindCell(point, cellId, parametric) == viskores::ErrorCode::Success)
    {
      // iterate through all the points of the cell and deposit with correct weight.
      auto indices = cellSet.GetIndices(cellId);
      auto rparametric = viskores::Vec3f{ 1, 1, 1 } - parametric;

      // deposit the scalar field value in proportion to the volume of the sub-hexahedron
      // the vertex is in.
      density.Add(indices[0], value * parametric[0] * parametric[1] * parametric[2]);
      density.Add(indices[1], value * rparametric[0] * parametric[1] * parametric[2]);
      density.Add(indices[2], value * rparametric[0] * rparametric[1] * parametric[2]);
      density.Add(indices[3], value * parametric[0] * rparametric[1] * parametric[2]);

      density.Add(indices[4], value * parametric[0] * parametric[1] * rparametric[2]);
      density.Add(indices[5], value * rparametric[0] * parametric[1] * rparametric[2]);
      density.Add(indices[6], value * rparametric[0] * rparametric[1] * rparametric[2]);
      density.Add(indices[7], value * parametric[0] * rparametric[1] * rparametric[2]);
    }

    // We simply ignore that particular particle when it is not in the mesh.
  }
};
} // worklet
} // viskores

namespace viskores
{
namespace filter
{
namespace density_estimate
{

VISKORES_CONT viskores::cont::DataSet ParticleDensityCloudInCell::DoExecute(
  const cont::DataSet& input)
{
  // Unlike ParticleDensityNGP, particle deposit mass on the grid points, thus it is natural to
  // return the density as PointField;
  auto uniform = viskores::cont::DataSetBuilderUniform::Create(
    this->Dimension + viskores::Id3{ 1, 1, 1 }, this->Origin, this->Spacing);

  viskores::cont::CellLocatorUniformGrid locator;
  locator.SetCellSet(uniform.GetCellSet());
  locator.SetCoordinates(uniform.GetCoordinateSystem());
  locator.Update();

  auto coords = input.GetCoordinateSystem().GetDataAsMultiplexer();

  auto resolveType = [&](const auto& concrete)
  {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;

    // We create an ArrayHandle and pass it to the Worklet as AtomicArrayInOut.
    // However, the ArrayHandle needs to be allocated and initialized first.
    viskores::cont::ArrayHandle<T> density;
    density.AllocateAndFill(uniform.GetNumberOfPoints(), 0);

    this->Invoke(viskores::worklet::CICWorklet{},
                 coords,
                 concrete,
                 locator,
                 uniform.GetCellSet().template AsCellSet<viskores::cont::CellSetStructured<3>>(),
                 density);

    if (DivideByVolume)
    {
      this->DoDivideByVolume(density);
    }

    uniform.AddField(viskores::cont::make_FieldPoint("density", density));
  };

  if (this->ComputeNumberDensity)
  {
    resolveType(viskores::cont::make_ArrayHandleConstant(viskores::FloatDefault{ 1 },
                                                         input.GetNumberOfPoints()));
  }
  else
  {
    this->CastAndCallScalarField(this->GetFieldFromDataSet(input), resolveType);
  }
  return uniform;
}
} // namespace density_estimate
} // namespace filter
} // namespace viskores
