//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Instance.h"

namespace viskores_device
{

Instance::Instance(ViskoresDeviceGlobalState* s)
  : Object(ANARI_INSTANCE, s)
{
}

Instance::~Instance() = default;

void Instance::commitParameters()
{
  m_id = getParam<uint32_t>("id", ~0u);
  m_xfm = getParam<mat4>("transform", mat4(linalg::identity));
  m_group = getParamObject<Group>("group");
  if (!m_group)
    reportMessage(ANARI_SEVERITY_WARNING, "missing 'group' on ANARIInstance");
}

void Instance::finalize()
{
  m_xfmInvRot = linalg::inverse(extractRotation(m_xfm));
}

uint32_t Instance::id() const
{
  return m_id;
}

const mat4& Instance::xfm() const
{
  return m_xfm;
}

const mat3& Instance::xfmInvRot() const
{
  return m_xfmInvRot;
}

bool Instance::xfmIsIdentity() const
{
  return xfm() == mat4(linalg::identity);
}

const Group* Instance::group() const
{
  return m_group.ptr;
}

Group* Instance::group()
{
  return m_group.ptr;
}

bool Instance::isValid() const
{
  return m_group;
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::Instance*);
