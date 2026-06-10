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

#include "RenderingSemaphore.h"
#include "viskores_device_math.h"
// helium
#include "helium/BaseGlobalDeviceState.h"
// std
#include <atomic>
#include <mutex>

namespace viskores_device
{

struct Frame;

struct ViskoresDeviceGlobalState : public helium::BaseGlobalDeviceState
{
  struct ObjectCounts
  {
    std::atomic<size_t> frames{ 0 };
    std::atomic<size_t> cameras{ 0 };
    std::atomic<size_t> renderers{ 0 };
    std::atomic<size_t> worlds{ 0 };
    std::atomic<size_t> instances{ 0 };
    std::atomic<size_t> groups{ 0 };
    std::atomic<size_t> surfaces{ 0 };
    std::atomic<size_t> geometries{ 0 };
    std::atomic<size_t> materials{ 0 };
    std::atomic<size_t> samplers{ 0 };
    std::atomic<size_t> volumes{ 0 };
    std::atomic<size_t> spatialFields{ 0 };
    std::atomic<size_t> arrays{ 0 };
    std::atomic<size_t> unknown{ 0 };
  } objectCounts;

  RenderingSemaphore renderingSemaphore;
  Frame* currentFrame{ nullptr };

  // Helper methods //

  ViskoresDeviceGlobalState(ANARIDevice d);
  void waitOnCurrentFrame() const;
};

// Helper functions/macros ////////////////////////////////////////////////////

inline ViskoresDeviceGlobalState* asViskoresDeviceState(helium::BaseGlobalDeviceState* s)
{
  return (ViskoresDeviceGlobalState*)s;
}

#define VISKORES_ANARI_TYPEFOR_SPECIALIZATION(type, anari_type) \
  namespace anari                                               \
  {                                                             \
  ANARI_TYPEFOR_SPECIALIZATION(type, anari_type);               \
  }

#define VISKORES_ANARI_TYPEFOR_DEFINITION(type) \
  namespace anari                               \
  {                                             \
  ANARI_TYPEFOR_DEFINITION(type);               \
  }

} // namespace viskores_device
