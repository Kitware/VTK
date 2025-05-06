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
#include <viskores/filter/density_estimate/ParticleDensityNearestGridPoint.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
class NGPWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn coords,
                                FieldIn field,
                                ExecObject locator,
                                AtomicArrayInOut density);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename Point, typename T, typename CellLocatorExecObj, typename AtomicArray>
  VISKORES_EXEC void operator()(const Point& point,
                                const T value,
                                const CellLocatorExecObj& locator,
                                AtomicArray& density) const
  {
    viskores::Id cellId{};
    viskores::Vec3f parametric;

    // Find the cell containing the point
    if (locator.FindCell(point, cellId, parametric) == viskores::ErrorCode::Success)
    {
      // deposit field value to density
      density.Add(cellId, value);
    }

    // We simply ignore that particular particle when it is not in the mesh.
  }
}; //NGPWorklet
} //worklet
} //viskores

namespace viskores
{
namespace filter
{
namespace density_estimate
{

VISKORES_CONT viskores::cont::DataSet ParticleDensityNearestGridPoint::DoExecute(
  const viskores::cont::DataSet& input)
{
  // TODO: it really doesn't need to be a UniformGrid, any CellSet with CellLocator will work.
  //  Make it another input rather an output generated.

  // We stores density as CellField which conforms to physicists' idea of particle density
  // better. However, VTK/Viskores's idea of "Image" Dataset and the ImageConnectivity filter
  // expect a PointField. For better separation of concerns, we create a uniform dataset
  // that has the cell dimension as expected and later convert the dataset to its dual.
  auto uniform = viskores::cont::DataSetBuilderUniform::Create(
    this->Dimension + viskores::Id3{ 1, 1, 1 }, this->Origin, this->Spacing);

  // Create a CellLocator
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
    density.AllocateAndFill(uniform.GetNumberOfCells(), 0);

    this->Invoke(viskores::worklet::NGPWorklet{}, coords, concrete, locator, density);

    if (DivideByVolume)
    {
      this->DoDivideByVolume(density);
    }

    uniform.AddField(viskores::cont::make_FieldCell("density", density));
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

  // Deposition of the input field to the output field is already mapping. No need to map other
  // fields.
  return uniform;
}
}
}
}
