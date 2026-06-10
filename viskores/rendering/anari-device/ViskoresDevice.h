//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#pragma once

// helium
#include "helium/BaseDevice.h"
// #include "ChangeObserverPtr.h"

#include "Object.h"

#include <viskores/rendering/anari-device/anari_library_viskores_export.h>

namespace viskores_device
{

struct ANARI_LIBRARY_VISKORES_EXPORT ViskoresDevice : public helium::BaseDevice
{
  /////////////////////////////////////////////////////////////////////////////
  // Main interface to accepting API calls
  /////////////////////////////////////////////////////////////////////////////

  // Data Arrays //////////////////////////////////////////////////////////////

  void* mapArray(ANARIArray) override;
  void unmapArray(ANARIArray) override;

  // API Objects //////////////////////////////////////////////////////////////

  ANARIArray1D newArray1D(const void* appMemory,
                          ANARIMemoryDeleter deleter,
                          const void* userdata,
                          ANARIDataType,
                          uint64_t numItems1) override;
  ANARIArray2D newArray2D(const void* appMemory,
                          ANARIMemoryDeleter deleter,
                          const void* userdata,
                          ANARIDataType,
                          uint64_t numItems1,
                          uint64_t numItems2) override;
  ANARIArray3D newArray3D(const void* appMemory,
                          ANARIMemoryDeleter deleter,
                          const void* userdata,
                          ANARIDataType,
                          uint64_t numItems1,
                          uint64_t numItems2,
                          uint64_t numItems3) override;
  ANARICamera newCamera(const char* type) override;
  ANARIFrame newFrame() override;
  ANARIGeometry newGeometry(const char* type) override;
  ANARIGroup newGroup() override;
  ANARIInstance newInstance(const char* type) override;
  ANARILight newLight(const char* type) override;
  ANARIMaterial newMaterial(const char* material_type) override;
  ANARIRenderer newRenderer(const char* type) override;
  ANARISampler newSampler(const char* type) override;
  ANARISpatialField newSpatialField(const char* type) override;
  ANARISurface newSurface() override;
  ANARIVolume newVolume(const char* type) override;
  ANARIWorld newWorld() override;

  // Query functions //////////////////////////////////////////////////////////

  const char** getObjectSubtypes(ANARIDataType objectType) override;
  const void* getObjectInfo(ANARIDataType objectType,
                            const char* objectSubtype,
                            const char* infoName,
                            ANARIDataType infoType) override;
  const void* getParameterInfo(ANARIDataType objectType,
                               const char* objectSubtype,
                               const char* parameterName,
                               ANARIDataType parameterType,
                               const char* infoName,
                               ANARIDataType infoType) override;

  // Object + Parameter Lifetime Management ///////////////////////////////////

  int getProperty(ANARIObject object,
                  const char* name,
                  ANARIDataType type,
                  void* mem,
                  uint64_t size,
                  uint32_t mask) override;

  /////////////////////////////////////////////////////////////////////////////
  // Helper/other functions and data members
  /////////////////////////////////////////////////////////////////////////////

  ViskoresDevice(ANARIStatusCallback defaultCallback, const void* userPtr);
  ViskoresDevice(ANARILibrary);
  ~ViskoresDevice() override;

  void initDevice();

  void deviceCommitParameters() override;
  int deviceGetProperty(const char* name,
                        ANARIDataType type,
                        void* mem,
                        uint64_t size,
                        uint32_t mask) override;

private:
  ViskoresDeviceGlobalState* deviceState() const;

  bool m_initialized{ false };
};

} // namespace viskores_device
