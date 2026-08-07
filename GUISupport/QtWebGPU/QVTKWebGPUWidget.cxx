// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "QVTKWebGPUWidget.h"

#include <QApplication>
#include <QResizeEvent>
#include <QtDebug>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <objc/message.h>
#include <objc/runtime.h>
#elif defined(VTK_USE_X) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
// Qt forward-declares `Display` in this header, and the display and window
// handles are handed to Dawn as `void*`/`uint64_t`. libX11 is therefore needed
// neither at compile time nor at link time.
#include <QtGui/qguiapplication_platform.h>
#endif

#include "QVTKInteractor.h"
#include "QVTKInteractorAdapter.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPURenderWindow.h"

#include <cstdint>
#include <string>

#include <webgpu/webgpu_cpp.h>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
struct QVTKWebGPUWidget::PlatformSurface
{
#ifdef _WIN32
  wgpu::SurfaceSourceWindowsHWND Descriptor;
#elif defined(__APPLE__)
  wgpu::SurfaceSourceMetalLayer Descriptor;
  void* MetalLayer = nullptr;
#elif defined(VTK_USE_Wayland)
  wgpu::SurfaceSourceWaylandSurface Descriptor;
#elif defined(VTK_USE_X)
  wgpu::SurfaceSourceXlibWindow Descriptor;
#endif
};

namespace
{
void EnsureWebGPUObjectFactoryPreference()
{
  constexpr const char* webgpuPreference = "RenderingBackend=WebGPU";
  std::string preferences = vtkObjectFactory::GetPreferences();
  if (preferences == webgpuPreference ||
    preferences.rfind(std::string(webgpuPreference) + ';', 0) == 0)
  {
    return;
  }

  std::string updatedPreferences = webgpuPreference;
  std::size_t start = 0;
  while (start < preferences.size())
  {
    const auto end = preferences.find(';', start);
    const auto entry =
      preferences.substr(start, end == std::string::npos ? std::string::npos : end - start);
    if (!entry.empty() && entry.rfind("RenderingBackend=", 0) != 0)
    {
      updatedPreferences += ';';
      updatedPreferences += entry;
    }
    if (end == std::string::npos)
    {
      break;
    }
    start = end + 1;
  }

  vtkObjectFactory::SetPreferences(updatedPreferences);
}
}

//------------------------------------------------------------------------------
QVTKWebGPUWidget::QVTKWebGPUWidget(QWidget* parentWdg, Qt::WindowFlags f)
  : Superclass(parentWdg, f)
  , DefaultCursor(QCursor(Qt::ArrowCursor))
  , Platform(std::make_unique<PlatformSurface>())
{
  EnsureWebGPUObjectFactoryPreference();

  // Use native window for direct surface access
  this->setAttribute(Qt::WA_NativeWindow);
  this->setAttribute(Qt::WA_PaintOnScreen);
  this->setAttribute(Qt::WA_NoSystemBackground);

  // default to strong focus
  this->setFocusPolicy(Qt::StrongFocus);
  this->setMouseTracking(true);

  // Create the render window
  this->RenderWindow = vtkSmartPointer<vtkWebGPURenderWindow>::New();

  // Create interactor adapter
  this->InteractorAdapter = std::make_unique<QVTKInteractorAdapter>(this);

  // Create a default interactor
  vtkNew<QVTKInteractor> iren;
  this->RenderWindow->SetInteractor(iren);

  // Set default interactor style
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  // Enable Qt gesture events
  this->grabGesture(Qt::PinchGesture);
  this->grabGesture(Qt::PanGesture);
  this->grabGesture(Qt::TapGesture);
  this->grabGesture(Qt::TapAndHoldGesture);
  this->grabGesture(Qt::SwipeGesture);
}

//------------------------------------------------------------------------------
QVTKWebGPUWidget::~QVTKWebGPUWidget()
{
  // Explicitly destroy the InteractorAdapter first to prevent it from receiving
  // events during widget destruction. When the unique_ptr deletes the adapter,
  // Qt sends a ChildRemoved event which could otherwise try to use the adapter.
  this->InteractorAdapter.reset();

  if (this->RenderWindow)
  {
    this->RenderWindow->SetCustomSurfaceDescriptor(nullptr);
    this->RenderWindow->Finalize();
  }
#ifdef __APPLE__
  // Release the Metal layer if we created one
  if (this->Platform->MetalLayer)
  {
    // Release
    ((void (*)(id, SEL))objc_msgSend)((id)this->Platform->MetalLayer, sel_registerName("release"));
    this->Platform->MetalLayer = nullptr;
  }
#endif
}

//------------------------------------------------------------------------------
vtkWebGPURenderWindow* QVTKWebGPUWidget::renderWindow() const
{
  return this->RenderWindow;
}

//------------------------------------------------------------------------------
QVTKInteractor* QVTKWebGPUWidget::interactor() const
{
  return this->RenderWindow ? QVTKInteractor::SafeDownCast(this->RenderWindow->GetInteractor())
                            : nullptr;
}

//------------------------------------------------------------------------------
void QVTKWebGPUWidget::setEnableHiDPI(bool enable)
{
  this->EnableHiDPI = enable;
  this->updateSize();
}

//------------------------------------------------------------------------------
void QVTKWebGPUWidget::setUnscaledDPI(int dpi)
{
  this->UnscaledDPI = dpi;
  this->updateSize();
}

//------------------------------------------------------------------------------
void QVTKWebGPUWidget::setCustomDevicePixelRatio(double cdpr)
{
  this->CustomDevicePixelRatio = cdpr;
  this->updateSize();
}

//------------------------------------------------------------------------------
double QVTKWebGPUWidget::effectiveDevicePixelRatio() const
{
  return this->CustomDevicePixelRatio > 0.0 ? this->CustomDevicePixelRatio
                                            : this->devicePixelRatioF();
}

//------------------------------------------------------------------------------
void QVTKWebGPUWidget::setDefaultCursor(const QCursor& cursor)
{
  this->DefaultCursor = cursor;
  this->setCursor(cursor);
}

//------------------------------------------------------------------------------
void QVTKWebGPUWidget::createSurfaceDescriptor()
{
#ifdef _WIN32
  // Windows: Use HWND
  HWND hwnd = reinterpret_cast<HWND>(this->winId());
  HINSTANCE hinstance = GetModuleHandle(nullptr);

  this->Platform->Descriptor.hwnd = hwnd;
  this->Platform->Descriptor.hinstance = hinstance;

  this->SurfaceDescriptor = {};
  this->SurfaceDescriptor.label = "VTK Qt WebGPU Widget Surface";
  this->SurfaceDescriptor.nextInChain = &this->Platform->Descriptor;

#elif defined(__APPLE__)
  // macOS: Create a Metal layer and get it from the view
  // We need to create a CAMetalLayer and set it on the NSView
  void* nsView = reinterpret_cast<void*>(this->winId());

  // Create CAMetalLayer using Objective-C runtime
  Class metalLayerClass = objc_getClass("CAMetalLayer");
  if (metalLayerClass)
  {
    this->Platform->MetalLayer =
      ((id(*)(Class, SEL))objc_msgSend)(metalLayerClass, sel_registerName("layer"));
    if (this->Platform->MetalLayer)
    {
      // Retain the layer
      ((void (*)(id, SEL))objc_msgSend)((id)this->Platform->MetalLayer, sel_registerName("retain"));

      // Set the layer on the view: [view setLayer:metalLayer]
      ((void (*)(id, SEL, id))objc_msgSend)(
        (id)nsView, sel_registerName("setLayer:"), (id)this->Platform->MetalLayer);

      // Set wantsLayer to YES: [view setWantsLayer:YES]
      ((void (*)(id, SEL, BOOL))objc_msgSend)((id)nsView, sel_registerName("setWantsLayer:"), YES);

      this->Platform->Descriptor.layer = this->Platform->MetalLayer;

      this->SurfaceDescriptor = {};
      this->SurfaceDescriptor.label = "VTK Qt WebGPU Widget Surface";
      this->SurfaceDescriptor.nextInChain = &this->Platform->Descriptor;
    }
  }

#elif defined(VTK_USE_Wayland)
  // Wayland surface
  // Note: Qt5/6 may not directly expose Wayland surface, this is a placeholder
  qWarning() << "Wayland support for QVTKWebGPUWidget is not yet fully implemented";

#elif defined(VTK_USE_X)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  // X11: `winId()` forces creation of the native window, which must happen
  // before the surface is created.
  const WId wid = this->winId();
  void* display = nullptr;
  if (auto* guiApp = qGuiApp)
  {
    if (auto* x11App = guiApp->nativeInterface<QNativeInterface::QX11Application>())
    {
      display = x11App->display();
    }
  }
  if (display == nullptr)
  {
    qWarning() << "QVTKWebGPUWidget: could not obtain the X11 Display connection; the Qt platform "
                  "plugin in use is not xcb.";
    return;
  }

  this->Platform->Descriptor.display = display;
  this->Platform->Descriptor.window = static_cast<uint64_t>(wid);

  this->SurfaceDescriptor = {};
  this->SurfaceDescriptor.label = "VTK Qt WebGPU Widget Surface";
  this->SurfaceDescriptor.nextInChain = &this->Platform->Descriptor;
#else
  qWarning() << "QVTKWebGPUWidget: the X11 surface path requires Qt 6";
#endif

#else
  qWarning() << "Unsupported platform for QVTKWebGPUWidget";
#endif
}

//------------------------------------------------------------------------------
void QVTKWebGPUWidget::initializeWebGPU()
{
  if (this->Initialized || !this->RenderWindow)
  {
    return;
  }

  // Create the surface descriptor from the native window handle
  this->createSurfaceDescriptor();

  // Set the custom surface descriptor on the render window
  this->RenderWindow->SetCustomSurfaceDescriptor(&this->SurfaceDescriptor);

  // Set the window size
  this->updateSize();

  // Initialize the render window
  this->RenderWindow->Initialize();

  // Initialize the interactor only after the render window is bound to the
  // Qt-provided native surface.
  if (auto* iren = this->interactor(); iren && !iren->GetInitialized())
  {
    iren->Initialize();
  }

  this->Initialized = true;
}

//------------------------------------------------------------------------------
void QVTKWebGPUWidget::updateSize()
{
  if (!this->RenderWindow)
  {
    return;
  }

  const double dpr = this->effectiveDevicePixelRatio();
  const int deviceWidth = static_cast<int>(this->width() * dpr);
  const int deviceHeight = static_cast<int>(this->height() * dpr);

  this->RenderWindow->SetSize(deviceWidth, deviceHeight);

  if (this->EnableHiDPI)
  {
    this->RenderWindow->SetDPI(static_cast<int>(this->UnscaledDPI * dpr));
  }
  else
  {
    this->RenderWindow->SetDPI(this->UnscaledDPI);
  }
}

//------------------------------------------------------------------------------
void QVTKWebGPUWidget::resizeEvent(QResizeEvent* event)
{
  this->Superclass::resizeEvent(event);

  if (!this->Initialized)
  {
    this->initializeWebGPU();
  }
  else
  {
    this->updateSize();
  }
}

//------------------------------------------------------------------------------
void QVTKWebGPUWidget::paintEvent(QPaintEvent* event)
{
  Q_UNUSED(event);

  if (this->InPaint)
  {
    return;
  }

  if (!this->Initialized)
  {
    this->initializeWebGPU();
  }

  if (this->RenderWindow && this->Initialized)
  {
    this->InPaint = true;
    this->RenderWindow->Render();
    this->InPaint = false;
  }
}

//------------------------------------------------------------------------------
bool QVTKWebGPUWidget::event(QEvent* evt)
{
  // Check if InteractorAdapter is valid before using it. During destruction,
  // Qt can send events (like ChildRemoved) after the adapter has been deleted.
  if (this->InteractorAdapter && this->RenderWindow)
  {
    // Get the interactor and check if it's valid before processing the event.
    // During widget destruction, the interactor may be null even though
    // InteractorAdapter hasn't been fully destroyed yet.
    vtkRenderWindowInteractor* iren = this->RenderWindow->GetInteractor();
    if (iren && this->InteractorAdapter->ProcessEvent(evt, iren))
    {
      return true;
    }
  }
  return this->Superclass::event(evt);
}

VTK_ABI_NAMESPACE_END
