//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Frame.h"
// std
#include <algorithm>
#include <chrono>
#include <random>
#include <thread>

#include <scene/World.h>

#include <viskores/cont/ArrayHandleBasic.h>

#include <viskores/cont/Invoker.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores_device
{

constexpr float toneMap(viskores::Float32 v)
{
  return std::pow(v, 1.f / 2.2f);
}

constexpr uint32_t cvt_uint32(const viskores::Float32& f)
{
  return static_cast<uint32_t>(255.f * std::clamp(f, 0.f, 1.f));
}

constexpr uint32_t cvt_uint32(const viskores::Vec4f_32& v)
{
  return (cvt_uint32(v[0]) << 0) | (cvt_uint32(v[1]) << 8) | (cvt_uint32(v[2]) << 16) |
    (cvt_uint32(v[3]) << 24);
}

constexpr uint32_t cvt_uint32_srgb(const viskores::Vec4f_32& v)
{
  return cvt_uint32(viskores::Vec4f_32(toneMap(v[0]), toneMap(v[1]), toneMap(v[2]), v[3]));
}

class ConvertToRGBA : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn inputArray, FieldOut outputArray);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  template <typename InFieldType, typename OutFieldType>
  VISKORES_EXEC void operator()(const InFieldType& inField, OutFieldType& outField) const
  {
    outField = cvt_uint32(inField);
  }
};

class ConvertToSRGBA : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn inputArray, FieldOut outputArray);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  template <typename InFieldType, typename OutFieldType>
  VISKORES_EXEC void operator()(const InFieldType& inField, OutFieldType& outField) const
  {
    outField = cvt_uint32_srgb(inField);
  }

  //  private:
  //   viskores::Float32 Exponent = 1.1f / 2.2f;
};

// Helper functions ///////////////////////////////////////////////////////////

template <typename R, typename TASK_T>
static std::future<R> async(TASK_T&& fcn)
{
  auto task = std::packaged_task<R()>(std::forward<TASK_T>(fcn));
  auto future = task.get_future();
  std::thread([task = std::move(task)]() mutable { task(); }).detach();
  return future;
}

template <typename R>
static bool is_ready(const std::future<R>& f)
{
  return !f.valid() || f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

// Frame definitions //////////////////////////////////////////////////////////

Frame::Frame(ViskoresDeviceGlobalState* s)
  : helium::BaseFrame(s)
{
}

Frame::~Frame()
{
  wait();
  deviceState()->objectCounts.frames--;
}

bool Frame::isValid() const
{
  return m_valid;
}

ViskoresDeviceGlobalState* Frame::deviceState() const
{
  return (ViskoresDeviceGlobalState*)helium::BaseObject::m_state;
}

void Frame::commitParameters()
{
  m_renderer = getParamObject<Renderer>("renderer");
  if (!m_renderer)
  {
    reportMessage(ANARI_SEVERITY_WARNING, "missing required parameter 'renderer' on frame");
  }

  m_camera = getParamObject<Camera>("camera");
  if (!m_camera)
  {
    reportMessage(ANARI_SEVERITY_WARNING, "missing required parameter 'camera' on frame");
  }

  m_world = getParamObject<World>("world");
  if (!m_world)
  {
    reportMessage(ANARI_SEVERITY_WARNING, "missing required parameter 'world' on frame");
  }

  m_valid = m_renderer && m_renderer->isValid() && m_camera && m_camera->isValid() && m_world &&
    m_world->isValid();

  m_colorType = getParam<anari::DataType>("channel.color", ANARI_UNKNOWN);
  m_depthType = getParam<anari::DataType>("channel.depth", ANARI_UNKNOWN);
  m_primIdType = getParam<anari::DataType>("channel.primitiveId", ANARI_UNKNOWN);
  m_objIdType = getParam<anari::DataType>("channel.objectId", ANARI_UNKNOWN);
  m_instIdType = getParam<anari::DataType>("channel.instanceId", ANARI_UNKNOWN);

  m_frameData.size = getParam<uint2>("size", uint2(10));
}

void Frame::finalize()
{
  this->Canvas =
    viskores::rendering::CanvasRayTracer(this->m_frameData.size[0], this->m_frameData.size[1]);
  this->Canvas.SetBackgroundColor(this->m_renderer->background());

  const auto numPixels = m_frameData.size[0] * m_frameData.size[1];

  m_perPixelBytes = 4 * (m_colorType == ANARI_FLOAT32_VEC4 ? 4 : 1);
  m_pixelBuffer.resize(numPixels * m_perPixelBytes);

  m_depthBuffer.resize(m_depthType == ANARI_FLOAT32 ? numPixels : 0);
  m_frameChanged = true;

  m_primIdBuffer.clear();
  m_objIdBuffer.clear();
  m_instIdBuffer.clear();

  if (m_primIdType == ANARI_UINT32)
    m_primIdBuffer.resize(numPixels);
  if (m_objIdType == ANARI_UINT32)
    m_objIdBuffer.resize(numPixels);
  if (m_instIdType == ANARI_UINT32)
    m_instIdBuffer.resize(numPixels);
}

bool Frame::getProperty(const std::string_view& name,
                        ANARIDataType type,
                        void* ptr,
                        uint64_t viskoresNotUsed(size),
                        uint32_t viskoresNotUsed(flags))
{
  if (type == ANARI_FLOAT32 && name == "duration")
  {
    helium::writeToVoidP(ptr, m_duration);
    return true;
  }

  return 0;
}

void Frame::renderFrame()
{
  this->refInc(helium::RefType::INTERNAL);

  auto* state = deviceState();
  state->waitOnCurrentFrame();
  state->currentFrame = this;

  m_future = async<void>(
    [&, state]()
    {
      auto start = std::chrono::steady_clock::now();
      state->renderingSemaphore.frameStart();
      state->commitBuffer.flush();

      if (!m_renderer)
      {
        reportMessage(ANARI_SEVERITY_ERROR, "skipping render of incomplete frame object");
        std::fill(m_pixelBuffer.begin(), m_pixelBuffer.end(), 0);
      }
      else
      {
        reportMessage(ANARI_SEVERITY_DEBUG, "rendering frame");

        const auto& instances = this->m_world->instances();
        auto camera = this->m_camera->camera(this->m_world->bounds());

#if 0
      std::cout << "\n\nANARI camera:" << std::endl;
      camera.Print();
#endif

        std::vector<Surface*> surfacesToRender;
        std::vector<Volume*> volumesToRender;

        for (const auto& instance : instances)
        {
          if (!instance->isValid())
          {
            reportMessage(ANARI_SEVERITY_DEBUG, "skip rendering invalid group");
            continue;
          }

          auto& surfaces = instance->group()->surfaces();
          surfacesToRender.insert(surfacesToRender.end(), surfaces.begin(), surfaces.end());

          auto& volumes = instance->group()->volumes();
          volumesToRender.insert(volumesToRender.end(), volumes.begin(), volumes.end());
        }

        this->Canvas.Clear();

        for (const auto& surface : surfacesToRender)
        {
          if (!surface || !surface->isValid())
          {
            reportMessage(ANARI_SEVERITY_DEBUG, "skip rendering invalid surface");
            continue;
          }
          surface->render(this->Canvas, camera);
        }

        for (const auto& volume : volumesToRender)
        {
          if (!volume || !volume->isValid())
          {
            reportMessage(ANARI_SEVERITY_DEBUG, "skip rendering invalid volume");
            continue;
          }
          volume->render(this->Canvas, camera);
        }
      }

      this->Canvas.BlendBackground();

      state->renderingSemaphore.frameEnd();
      auto end = std::chrono::steady_clock::now();
      m_duration = std::chrono::duration<float>(end - start).count();
    });
}

void* Frame::map(std::string_view channel,
                 uint32_t* width,
                 uint32_t* height,
                 ANARIDataType* pixelType)
{
  wait();

  *width = m_frameData.size[0];
  *height = m_frameData.size[1];

  if (channel == "channel.color")
  {
    *pixelType = this->m_colorType;
    if (this->m_colorType == ANARI_FLOAT32_VEC4)
    {
      // change this to GetReadPointer().
      viskores::cont::ArrayHandleBasic<viskores::Vec4f_32> basicArray =
        this->Canvas.GetColorBuffer();
      return basicArray.GetWritePointer();
    }
    else if (this->m_colorType == ANARI_UFIXED8_VEC4)
    {
      this->m_intFrameBuffer.Allocate(*width * *height);
      ConvertToRGBA worklet;
      viskores::cont::Invoker invoker;
      invoker(worklet, this->Canvas.GetColorBuffer(), this->m_intFrameBuffer);
      viskores::cont::ArrayHandleBasic<viskores::UInt32> basicArray = this->m_intFrameBuffer;
      return basicArray.GetWritePointer();
    }
    else if (this->m_colorType == ANARI_UFIXED8_RGBA_SRGB)
    {
      this->m_intFrameBuffer.Allocate(*width * *height);
      ConvertToSRGBA worklet;
      viskores::cont::Invoker invoker;
      invoker(worklet, this->Canvas.GetColorBuffer(), this->m_intFrameBuffer);

      viskores::cont::ArrayHandleBasic<viskores::UInt32> basicArray = this->m_intFrameBuffer;
      // Note: Although we are returning a non-const pointer, this is
      // essentially a mistake in the ANARI API. Client code is not supposed
      // to modify the buffer.
      return const_cast<viskores::UInt32*>(basicArray.GetReadPointer());
    }
  }
  else if (channel == "channel.depth")
  {
    *pixelType = ANARI_FLOAT32;
    viskores::cont::ArrayHandleBasic<viskores::Float32> basicArray = this->Canvas.GetDepthBuffer();
    // Note: Although we are returning a non-const pointer, this is
    // essentially a mistake in the ANARI API. Client code is not supposed
    // to modify the buffer.
    return const_cast<viskores::Float32*>(basicArray.GetReadPointer());
  }
  else if (channel == "channel.primitiveId" && !m_primIdBuffer.empty())
  {
    *pixelType = ANARI_UINT32;
    return m_primIdBuffer.data();
  }
  else if (channel == "channel.objectId" && !m_objIdBuffer.empty())
  {
    *pixelType = ANARI_UINT32;
    return m_objIdBuffer.data();
  }
  else if (channel == "channel.instanceId" && !m_instIdBuffer.empty())
  {
    *pixelType = ANARI_UINT32;
    return m_instIdBuffer.data();
  }

  *width = 0;
  *height = 0;
  *pixelType = ANARI_UNKNOWN;
  return nullptr;
}

void Frame::unmap(std::string_view /*channel*/)
{
  // no-op
}

int Frame::frameReady(ANARIWaitMask m)
{
  if (m == ANARI_NO_WAIT)
    return ready();
  else
  {
    wait();
    return 1;
  }
}

void Frame::discard()
{
  this->m_intFrameBuffer.ReleaseResources();
}

bool Frame::ready() const
{
  return is_ready(m_future);
}

void Frame::wait()
{
  if (m_future.valid())
  {
    m_future.get();
    this->refDec(helium::RefType::INTERNAL);
    if (deviceState()->currentFrame == this)
      deviceState()->currentFrame = nullptr;
  }
}

void Frame::writeSample(int x, int y, const PixelSample& s)
{
  const auto idx = y * m_frameData.size[0] + x;
  auto* color = m_pixelBuffer.data() + (idx * m_perPixelBytes);
  switch (m_colorType)
  {
    case ANARI_UFIXED8_VEC4:
    {
      auto c = cvt_uint32(s.color);
      std::memcpy(color, &c, sizeof(c));
      break;
    }
    case ANARI_UFIXED8_RGBA_SRGB:
    {
      auto c = cvt_uint32_srgb(s.color);
      std::memcpy(color, &c, sizeof(c));
      break;
    }
    case ANARI_FLOAT32_VEC4:
    {
      std::memcpy(color, &s.color, sizeof(s.color));
      break;
    }
    default:
      break;
  }
  if (!m_depthBuffer.empty())
    m_depthBuffer[idx] = s.depth;
  if (!m_primIdBuffer.empty())
    m_primIdBuffer[idx] = s.primId;
  if (!m_objIdBuffer.empty())
    m_objIdBuffer[idx] = s.objId;
  if (!m_instIdBuffer.empty())
    m_instIdBuffer[idx] = s.instId;
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::Frame*);
