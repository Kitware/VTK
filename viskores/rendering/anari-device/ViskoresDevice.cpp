//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "ViskoresDevice.h"

#include "array/Array1D.h"
#include "array/Array2D.h"
#include "array/Array3D.h"
#include "array/ObjectArray.h"
#include "frame/Frame.h"
#include "scene/volume/spatial_field/SpatialField.h"

#include "anari_library_viskores_queries.h"

#include <algorithm>
#include <viskores/Version.h>

namespace viskores_device
{

///////////////////////////////////////////////////////////////////////////////
// Helper functions ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename HANDLE_T, typename OBJECT_T>
inline HANDLE_T getHandleForAPI(OBJECT_T* object)
{
  return (HANDLE_T)object;
}

template <typename OBJECT_T, typename HANDLE_T, typename... Args>
inline HANDLE_T createObjectForAPI(ViskoresDeviceGlobalState* s, Args&&... args)
{
  return getHandleForAPI<HANDLE_T>(new OBJECT_T(s, std::forward<Args>(args)...));
}

///////////////////////////////////////////////////////////////////////////////
// ViskoresDevice definitions
// /////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Data Arrays ////////////////////////////////////////////////////////////////

void* ViskoresDevice::mapArray(ANARIArray a)
{
  deviceState()->renderingSemaphore.arrayMapAcquire();
  return helium::BaseDevice::mapArray(a);
}

void ViskoresDevice::unmapArray(ANARIArray a)
{
  helium::BaseDevice::unmapArray(a);
  deviceState()->renderingSemaphore.arrayMapRelease();
}

// API Objects ////////////////////////////////////////////////////////////////

ANARIArray1D ViskoresDevice::newArray1D(const void* appMemory,
                                        ANARIMemoryDeleter deleter,
                                        const void* userData,
                                        ANARIDataType type,
                                        uint64_t numItems)
{
  initDevice();

  Array1DMemoryDescriptor md;
  md.appMemory = appMemory;
  md.deleter = deleter;
  md.deleterPtr = userData;
  md.elementType = type;
  md.numItems = numItems;

  if (anari::isObject(type))
    return createObjectForAPI<ObjectArray, ANARIArray1D>(deviceState(), md);
  else
    return createObjectForAPI<Array1D, ANARIArray1D>(deviceState(), md);
}

ANARIArray2D ViskoresDevice::newArray2D(const void* appMemory,
                                        ANARIMemoryDeleter deleter,
                                        const void* userData,
                                        ANARIDataType type,
                                        uint64_t numItems1,
                                        uint64_t numItems2)
{
  initDevice();

  Array2DMemoryDescriptor md;
  md.appMemory = appMemory;
  md.deleter = deleter;
  md.deleterPtr = userData;
  md.elementType = type;
  md.numItems1 = numItems1;
  md.numItems2 = numItems2;

  return createObjectForAPI<Array2D, ANARIArray2D>(deviceState(), md);
}

ANARIArray3D ViskoresDevice::newArray3D(const void* appMemory,
                                        ANARIMemoryDeleter deleter,
                                        const void* userData,
                                        ANARIDataType type,
                                        uint64_t numItems1,
                                        uint64_t numItems2,
                                        uint64_t numItems3)
{
  initDevice();

  Array3DMemoryDescriptor md;
  md.appMemory = appMemory;
  md.deleter = deleter;
  md.deleterPtr = userData;
  md.elementType = type;
  md.numItems1 = numItems1;
  md.numItems2 = numItems2;
  md.numItems3 = numItems3;

  return createObjectForAPI<Array3D, ANARIArray3D>(deviceState(), md);
}

ANARICamera ViskoresDevice::newCamera(const char* subtype)
{
  initDevice();
  return getHandleForAPI<ANARICamera>(Camera::createInstance(subtype, deviceState()));
}

ANARIFrame ViskoresDevice::newFrame()
{
  initDevice();
  return createObjectForAPI<Frame, ANARIFrame>(deviceState());
}

ANARIGeometry ViskoresDevice::newGeometry(const char* subtype)
{
  initDevice();
  return getHandleForAPI<ANARIGeometry>(Geometry::createInstance(subtype, deviceState()));
}

ANARIGroup ViskoresDevice::newGroup()
{
  initDevice();
  return createObjectForAPI<Group, ANARIGroup>(deviceState());
}

ANARIInstance ViskoresDevice::newInstance(const char* /*subtype*/)
{
  initDevice();
  return createObjectForAPI<Instance, ANARIInstance>(deviceState());
}

ANARILight ViskoresDevice::newLight(const char* subtype)
{
  initDevice();
  return getHandleForAPI<ANARILight>(Light::createInstance(subtype, deviceState()));
}

ANARIMaterial ViskoresDevice::newMaterial(const char* subtype)
{
  initDevice();
  return getHandleForAPI<ANARIMaterial>(Material::createInstance(subtype, deviceState()));
}

ANARIRenderer ViskoresDevice::newRenderer(const char* subtype)
{
  initDevice();
  return getHandleForAPI<ANARIRenderer>(Renderer::createInstance(subtype, deviceState()));
}

ANARISampler ViskoresDevice::newSampler(const char* subtype)
{
  initDevice();
  return getHandleForAPI<ANARISampler>(Sampler::createInstance(subtype, deviceState()));
}

ANARISpatialField ViskoresDevice::newSpatialField(const char* subtype)
{
  initDevice();
  return getHandleForAPI<ANARISpatialField>(SpatialField::createInstance(subtype, deviceState()));
}

ANARISurface ViskoresDevice::newSurface()
{
  initDevice();
  return createObjectForAPI<Surface, ANARISurface>(deviceState());
}

ANARIVolume ViskoresDevice::newVolume(const char* subtype)
{
  initDevice();
  return getHandleForAPI<ANARIVolume>(Volume::createInstance(subtype, deviceState()));
}

ANARIWorld ViskoresDevice::newWorld()
{
  initDevice();
  return createObjectForAPI<World, ANARIWorld>(deviceState());
}

// Query functions ////////////////////////////////////////////////////////////

const char** ViskoresDevice::getObjectSubtypes(ANARIDataType objectType)
{
  return viskores_device::query_object_types(objectType);
}

const void* ViskoresDevice::getObjectInfo(ANARIDataType objectType,
                                          const char* objectSubtype,
                                          const char* infoName,
                                          ANARIDataType infoType)
{
  return viskores_device::query_object_info(objectType, objectSubtype, infoName, infoType);
}

const void* ViskoresDevice::getParameterInfo(ANARIDataType objectType,
                                             const char* objectSubtype,
                                             const char* parameterName,
                                             ANARIDataType parameterType,
                                             const char* infoName,
                                             ANARIDataType infoType)
{
  return viskores_device::query_param_info(
    objectType, objectSubtype, parameterName, parameterType, infoName, infoType);
}

// Object + Parameter Lifetime Management /////////////////////////////////////

int ViskoresDevice::getProperty(ANARIObject object,
                                const char* name,
                                ANARIDataType type,
                                void* mem,
                                uint64_t size,
                                uint32_t mask)
{
  if (mask == ANARI_WAIT)
  {
    auto lock = scopeLockObject();
    deviceState()->waitOnCurrentFrame();
  }

  return helium::BaseDevice::getProperty(object, name, type, mem, size, mask);
}


// Other ViskoresDevice definitions
// /////////////////////////////////////////////

ViskoresDevice::ViskoresDevice(ANARIStatusCallback cb, const void* ptr)
  : helium::BaseDevice(cb, ptr)
{
  m_state = std::make_unique<ViskoresDeviceGlobalState>(this_device());
  deviceCommitParameters();
}

ViskoresDevice::ViskoresDevice(ANARILibrary l)
  : helium::BaseDevice(l)
{
  m_state = std::make_unique<ViskoresDeviceGlobalState>(this_device());
  deviceCommitParameters();
}

ViskoresDevice::~ViskoresDevice()
{
  reportMessage(ANARI_SEVERITY_DEBUG, "destroying Viskores device (%p)", this);

  auto& state = *deviceState();
  state.commitBuffer.clear();
}

void ViskoresDevice::initDevice()
{
  if (m_initialized)
    return;

  reportMessage(ANARI_SEVERITY_DEBUG, "initializing Viskores device (%p)", this);

  m_initialized = true;
}

void ViskoresDevice::deviceCommitParameters()
{
  helium::BaseDevice::deviceCommitParameters();
}

int ViskoresDevice::deviceGetProperty(const char* name,
                                      ANARIDataType type,
                                      void* mem,
                                      uint64_t size,
                                      uint32_t mask)
{
  std::string_view prop = name;
  if (prop == "extension" && type == ANARI_STRING_LIST)
  {
    helium::writeToVoidP(mem, query_extensions());
    return 1;
  }
  else if (prop == "viskores" && type == ANARI_BOOL)
  {
    helium::writeToVoidP(mem, true);
    return 1;
  }
  else if ((prop == "version") && (type == ANARI_INT32))
  {
    viskores::Int32 version =
      (VISKORES_VERSION_MAJOR * 10000) + (VISKORES_VERSION_MINOR * 100) + VISKORES_VERSION_PATCH;
    helium::writeToVoidP(mem, version);
  }
  else if ((prop == "version.major") && (type == ANARI_INT32))
  {
    helium::writeToVoidP(mem, viskores::Int32(VISKORES_VERSION_MAJOR));
    return 1;
  }
  else if ((prop == "version.minor") && (type == ANARI_INT32))
  {
    helium::writeToVoidP(mem, viskores::Int32(VISKORES_VERSION_MINOR));
    return 1;
  }
  else if ((prop == "version.patch") && (type == ANARI_INT32))
  {
    helium::writeToVoidP(mem, viskores::Int32(VISKORES_VERSION_PATCH));
    return 1;
  }
  else if (prop == "version.name" && type == ANARI_STRING)
  {
    std::memset(mem, 0, size);
    std::memcpy(
      mem, VISKORES_VERSION_FULL, std::min(uint64_t(sizeof(VISKORES_VERSION_FULL)), size - 1));
    return 1;
  }

  return BaseDevice::deviceGetProperty(name, type, mem, size, mask);
}

ViskoresDeviceGlobalState* ViskoresDevice::deviceState() const
{
  return (ViskoresDeviceGlobalState*)helium::BaseDevice::m_state.get();
}

} // namespace viskores_device
