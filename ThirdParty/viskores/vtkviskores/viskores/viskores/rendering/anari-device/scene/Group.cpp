//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Group.h"
// std
#include <iterator>

namespace viskores_device
{

Group::Group(ViskoresDeviceGlobalState* s)
  : Object(ANARI_GROUP, s)
  , m_surfaceData(this)
  , m_volumeData(this)
{
}

Group::~Group() = default;

bool Group::getProperty(const std::string_view& name,
                        ANARIDataType type,
                        void* ptr,
                        uint64_t size,
                        uint32_t flags)
{
  return Object::getProperty(name, type, ptr, size, flags);
}

void Group::commitParameters()
{
  m_surfaceData = getParamObject<ObjectArray>("surface");
  m_volumeData = getParamObject<ObjectArray>("volume");
}

void Group::finalize()
{
  m_surfaces.clear();
  m_volumes.clear();

  if (m_surfaceData)
  {
    m_surfaceData->addChangeObserver(this);
    std::transform(m_surfaceData->handlesBegin(),
                   m_surfaceData->handlesEnd(),
                   std::back_inserter(m_surfaces),
                   [](auto* o) { return (Surface*)o; });
  }

  if (m_volumeData)
  {
    m_volumeData->addChangeObserver(this);
    std::transform(m_volumeData->handlesBegin(),
                   m_volumeData->handlesEnd(),
                   std::back_inserter(m_volumes),
                   [](auto* o) { return (Volume*)o; });
  }
}

const std::vector<Surface*>& Group::surfaces() const
{
  return m_surfaces;
}

const std::vector<Volume*>& Group::volumes() const
{
  return m_volumes;
}

static inline float clampIt(const float& a, const range_t<float>& r)
{
  if (a < r.lower)
    return r.lower;
  if (a > r.upper)
    return r.upper;
  return a;
}

static inline float CLAMP(const float& a, const box1& b)
{
  if (a < b.lower)
    return b.lower;
  else if (a > b.upper)
    return b.upper;

  return a;
}

static inline float3 MIN(const float3& a, const float3& b)
{
  float3 res(std::min(a[0], b[0]), std::min(a[1], b[1]), std::min(a[2], b[2]));
  return res;
}
static inline float3 MAX(const float3& a, const float3& b)
{
  float3 res(std::max(a[0], b[0]), std::max(a[1], b[1]), std::max(a[2], b[2]));
  return res;
}

static inline float MINELEM(const float3& a)
{
  return std::min(a[0], std::min(a[1], a[2]));
}
static inline float MAXELEM(const float3& a)
{
  return std::max(a[0], std::max(a[1], a[2]));
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::Group*);
