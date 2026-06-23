//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Sampler.h"
#include "Image1DSampler.h"
namespace
{

struct UnknownSampler : viskores_device::Sampler
{
  UnknownSampler(viskores_device::ViskoresDeviceGlobalState* d);

  void commitParameters() override;
  void finalize() override;

  bool isValid() const override;

  bool getColors(const viskores::cont::DataSet& data,
                 viskores::cont::Field& field,
                 viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const override;
};

UnknownSampler::UnknownSampler(viskores_device::ViskoresDeviceGlobalState* d)
  : Sampler(d)
{
}

void UnknownSampler::commitParameters()
{
  // invalid
}
void UnknownSampler::finalize()
{
  // invalid
}

bool UnknownSampler::isValid() const
{
  return false;
}

bool UnknownSampler::getColors(const viskores::cont::DataSet&,
                               viskores::cont::Field&,
                               viskores::cont::ArrayHandle<viskores::Vec4f_32>&) const
{
  // invalid
  return false;
}

} // anonymous namespace

namespace viskores_device
{

Sampler::Sampler(ViskoresDeviceGlobalState* s)
  : Object(ANARI_SAMPLER, s)
{
}

Sampler::~Sampler() = default;

Sampler* Sampler::createInstance(std::string_view subtype, ViskoresDeviceGlobalState* s)
{
  if (subtype == "image1D")
  {
    return new Image1DSampler(s);
  }
  else
  {
    return new UnknownSampler(s);
  }
}

void Sampler::commitParameters()
{
  mat4 outTransform = this->getParam("outTransform", mat4(linalg::identity));
  this->m_outTransform = toViskoresMatrix(outTransform);
  float4 outOffset = this->getParam("outOffset", float4(0.f, 0.f, 0.f, 0.f));
  this->m_outOffset = { outOffset[0], outOffset[1], outOffset[2], outOffset[3] };
}

void Sampler::finalize()
{
  // no-op
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::Sampler*);
