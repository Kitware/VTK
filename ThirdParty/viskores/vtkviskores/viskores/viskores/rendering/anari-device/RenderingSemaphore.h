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

#include <condition_variable>
#include <mutex>

namespace viskores_device
{

struct RenderingSemaphore
{
  RenderingSemaphore() = default;

  void arrayMapAcquire();
  void arrayMapRelease();

  void frameStart();
  void frameEnd();

private:
  std::mutex m_mutex;
  std::condition_variable m_conditionArrays;
  std::condition_variable m_conditionFrame;
  unsigned long m_arraysMapped{ 0 };
  bool m_frameInFlight{ false };
};

// Inlined definitions ////////////////////////////////////////////////////////

inline void RenderingSemaphore::arrayMapAcquire()
{
  std::unique_lock<std::mutex> frameLock(m_mutex);
  m_conditionFrame.wait(frameLock, [&]() { return !m_frameInFlight; });
  m_arraysMapped++;
}

inline void RenderingSemaphore::arrayMapRelease()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_arraysMapped--;
  if (m_arraysMapped == 0)
    m_conditionArrays.notify_one();
}

inline void RenderingSemaphore::frameStart()
{
  std::unique_lock<std::mutex> arraysLock(m_mutex);
  m_conditionArrays.wait(arraysLock, [&]() { return m_arraysMapped == 0; });
  m_frameInFlight = true;
}

inline void RenderingSemaphore::frameEnd()
{
  m_frameInFlight = false;
  m_conditionFrame.notify_all();
}

} // namespace viskores_device
