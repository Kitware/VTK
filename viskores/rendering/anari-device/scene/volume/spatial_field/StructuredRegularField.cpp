//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "StructuredRegularField.h"
// Viskores
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DataSetBuilderUniform.h>

namespace viskores_device
{

StructuredRegularField::StructuredRegularField(ViskoresDeviceGlobalState* d)
  : SpatialField(d)
  , m_dataArray(this)
{
}

void StructuredRegularField::commitParameters()
{
  // Stashing these in a ChangeObserverPtr means that commit will be
  // called again if the array contents change.
  this->m_dataArray = getParamObject<Array3D>("data");

  float3 origin = getParam("origin", float3{ 0, 0, 0 });
  float3 spacing = getParam("spacing", float3{ 1, 1, 1 });
  uint3 dimensions = this->m_dataArray->size();

  this->m_dataSet = viskores::cont::DataSetBuilderUniform::Create(
    viskores::Id3{ static_cast<viskores::Id>(dimensions[0]),
                   static_cast<viskores::Id>(dimensions[1]),
                   static_cast<viskores::Id>(dimensions[2]) },
    viskores::Vec3f{ origin[0], origin[1], origin[2] },
    viskores::Vec3f{ spacing[0], spacing[1], spacing[2] });
}

void StructuredRegularField::finalize()
{
  // Viskores volume render only supports float fields in volume rendering. Convert
  // if necessary.
  viskores::cont::UnknownArrayHandle viskoresArray = this->m_dataArray->dataAsViskoresArray();
  if (!viskoresArray.IsValueType<viskores::Float32>() &&
      !viskoresArray.IsValueType<viskores::Float64>())
  {
    viskores::cont::ArrayHandle<viskores::FloatDefault> castArray;
    viskores::cont::ArrayCopy(viskoresArray, castArray);
    viskoresArray = castArray;
  }
  this->m_dataSet.AddPointField("data", viskoresArray);

  // this->m_dataSet.PrintSummary(std::cout);
}

} // namespace viskores_device
