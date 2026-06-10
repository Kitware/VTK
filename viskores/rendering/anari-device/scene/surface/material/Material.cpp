//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Material.h"

#include "MatteMaterial.h"

namespace
{

struct UnknownMaterial : viskores_device::Material
{
  UnknownMaterial(viskores_device::ViskoresDeviceGlobalState* d);

  void commitParameters() override;
  void finalize() override;

  bool isValid() const override;

  void getColors(const viskores::cont::DataSet& data,
                 viskores::cont::Field& field,
                 viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const override;
};

UnknownMaterial::UnknownMaterial(viskores_device::ViskoresDeviceGlobalState* d)
  : Material(d)
{
}

void UnknownMaterial::commitParameters()
{
  // invalid
}
void UnknownMaterial::finalize()
{
  // invalid
}

bool UnknownMaterial::isValid() const
{
  return false;
}

void UnknownMaterial::getColors(const viskores::cont::DataSet&,
                                viskores::cont::Field&,
                                viskores::cont::ArrayHandle<viskores::Vec4f_32>&) const
{
  // invalid
}

} // anonymous namespace

namespace viskores_device
{

Material::Material(ViskoresDeviceGlobalState* s)
  : Object(ANARI_MATERIAL, s)
{
}

Material::~Material() = default;

Material* Material::createInstance(std::string_view subtype, ViskoresDeviceGlobalState* s)
{
  if (subtype == "matte")
  {
    return new MatteMaterial(s);
  }
  else
  {
    return new UnknownMaterial(s);
  }
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::Material*);
