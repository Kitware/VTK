//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "World.h"
#include "surface/Surface.h"
#include "volume/Volume.h"

namespace viskores_device
{

World::World(ViskoresDeviceGlobalState* s)
  : Object(ANARI_WORLD, s)
  , m_zeroSurfaceData(this)
  , m_zeroVolumeData(this)
  , m_instanceData(this)
{
  m_zeroGroup = new Group(s);
  m_zeroInstance = new Instance(s);
  m_zeroInstance->setParamDirect("group", m_zeroGroup.ptr);

  // never any public ref to these objects
  m_zeroGroup->refDec(helium::RefType::PUBLIC);
  m_zeroInstance->refDec(helium::RefType::PUBLIC);
}

World::~World() = default;

bool World::getProperty(const std::string_view& name,
                        ANARIDataType type,
                        void* ptr,
                        uint64_t size,
                        uint32_t flags)
{
  if (name == "bounds" && type == ANARI_FLOAT32_BOX3)
  {
    viskores::Bounds bounds = this->bounds();
    viskores::Vec3f_32 anariBounds[] = { viskores::Vec3f_32(bounds.MinCorner()),
                                         viskores::Vec3f_32(bounds.MaxCorner()) };
    std::memcpy(ptr, &anariBounds, sizeof(anariBounds));
    return true;
  }
  return Object::getProperty(name, type, ptr, size, flags);
}

void World::commitParameters()
{
  m_zeroSurfaceData = getParamObject<ObjectArray>("surface");
  m_zeroVolumeData = getParamObject<ObjectArray>("volume");

  m_addZeroInstance = m_zeroSurfaceData || m_zeroVolumeData;
  if (m_addZeroInstance)
  {
    reportMessage(ANARI_SEVERITY_DEBUG, "viskores_device::World will add zero instance");
  }

  if (m_zeroSurfaceData)
  {
    reportMessage(ANARI_SEVERITY_DEBUG,
                  "viskores_device::World found %zu surfaces in zero instance",
                  m_zeroSurfaceData->size());
    m_zeroGroup->setParamDirect("surface", getParamDirect("surface"));
  }
  else
  {
    m_zeroGroup->removeParam("surface");
  }

  if (m_zeroVolumeData)
  {
    reportMessage(ANARI_SEVERITY_DEBUG,
                  "viskores_device::World found %zu volumes in zero instance",
                  m_zeroVolumeData->size());
    m_zeroGroup->setParamDirect("volume", getParamDirect("volume"));
  }
  else
    m_zeroGroup->removeParam("volume");

  m_zeroInstance->setParam("id", getParam<uint32_t>("id", ~0u));

  m_zeroGroup->commitParameters();
  m_zeroGroup->finalize();
  m_zeroInstance->commitParameters();
  m_zeroInstance->finalize();

  m_instanceData = getParamObject<ObjectArray>("instance");
}

void World::finalize()
{
  m_instances.clear();

  if (m_instanceData)
  {
    m_instanceData->removeAppendedHandles();
    if (m_addZeroInstance)
      m_instanceData->appendHandle(m_zeroInstance.ptr);
    std::for_each(m_instanceData->handlesBegin(),
                  m_instanceData->handlesEnd(),
                  [&](auto* o)
                  {
                    if (o && o->isValid())
                      m_instances.push_back((Instance*)o);
                  });
  }
  else if (m_addZeroInstance)
    m_instances.push_back(m_zeroInstance.ptr);

  if (m_instanceData)
    m_instanceData->addChangeObserver(this);
  if (m_zeroSurfaceData)
    m_zeroSurfaceData->addChangeObserver(this);
}

viskores::Bounds World::bounds() const
{
  // It would be better to compute the bounds in `finalize` once so that it can
  // be reused. However, `finalize` is not refreshed if groups in the instance
  // are updated (thus potentially changing the bounds) because the world object
  // is not modified directly.
  viskores::Bounds bounds;
  for (auto&& instance : this->instances())
  {
    if (!instance->isValid())
    {
      reportMessage(ANARI_SEVERITY_DEBUG, "skip bounds check on invalid group");
      continue;
    }
    for (auto&& surface : instance->group()->surfaces())
    {
      if (!surface || !surface->isValid())
      {
        reportMessage(ANARI_SEVERITY_DEBUG, "skip bounds check on invalid surface");
        continue;
      }
      bounds.Include(surface->bounds());
    }
    for (auto&& volume : instance->group()->volumes())
    {
      if (!volume || !volume->isValid())
      {
        reportMessage(ANARI_SEVERITY_DEBUG, "skip bounds check on invalid volume");
        continue;
      }
      bounds.Include(volume->bounds());
    }
  }

  return bounds;
}

const std::vector<Instance*>& World::instances() const
{
  return m_instances;
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::World*);
