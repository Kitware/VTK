//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Volume.h"
#include "TransferFunction1D.h"

namespace
{

struct UnknownVolume : public viskores_device::Volume
{
  UnknownVolume(viskores_device::ViskoresDeviceGlobalState* d);

  viskores::Bounds bounds() const override;
  void render(viskores::rendering::Canvas& canvas,
              const viskores::rendering::Camera& camera) const override;
  bool isValid() const override;
};

UnknownVolume::UnknownVolume(viskores_device::ViskoresDeviceGlobalState* d)
  : Volume(d)
{
}

viskores::Bounds UnknownVolume::bounds() const
{
  return viskores::Bounds{};
}

void UnknownVolume::render(viskores::rendering::Canvas&, const viskores::rendering::Camera&) const
{
}

bool UnknownVolume::isValid() const
{
  return false;
}

} // anonymous namespace

namespace viskores_device
{

Volume::Volume(ViskoresDeviceGlobalState* s)
  : Object(ANARI_VOLUME, s)
{
}

Volume::~Volume() = default;

Volume* Volume::createInstance(std::string_view subtype, ViskoresDeviceGlobalState* s)
{
  if (subtype == "transferFunction1D")
    return new TransferFunction1D(s);
  else
    return new UnknownVolume(s);
}

void Volume::commitParameters()
{
  m_id = getParam<uint32_t>("id", ~0u);
}

void Volume::finalize()
{
  // no-op
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::Volume*);
