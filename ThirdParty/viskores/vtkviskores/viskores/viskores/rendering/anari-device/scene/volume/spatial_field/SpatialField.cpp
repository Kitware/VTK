//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "SpatialField.h"
// Subtypes
#include "StructuredRegularField.h"

namespace viskores_device
{

SpatialField::SpatialField(ViskoresDeviceGlobalState* s)
  : Object(ANARI_SPATIAL_FIELD, s)
{
}

SpatialField::~SpatialField() = default;

SpatialField* SpatialField::createInstance(std::string_view subtype, ViskoresDeviceGlobalState* s)
{
  if (subtype == "structuredRegular")
  {
    return new StructuredRegularField(s);
  }
  else
  {
    return new UnknownSpatialField(s);
  }
}

UnknownSpatialField::UnknownSpatialField(ViskoresDeviceGlobalState* d)
  : SpatialField(d)
{
}

void UnknownSpatialField::commitParameters()
{
  // invalid
}
void UnknownSpatialField::finalize()
{
  // invalid
}

bool UnknownSpatialField::isValid() const
{
  return false;
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::SpatialField*);
