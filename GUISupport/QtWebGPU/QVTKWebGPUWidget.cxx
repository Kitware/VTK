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
#elif defined(VTK_USE_X)
#include <QGuiApplication>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <qpa/qplatformnativeinterface.h>
#else
#include <QX11Info>
#endif
#include <X11/Xlib.h>
#endif

#include "QVTKInteractor.h"
#include "QVTKInteractorAdapter.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkWebGPURenderWindow.h"

#include <webgpu/webgpu_cpp.h>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
QVTKWebGPUWidget::QVTKWebGPUWidget(QWidget* parentWdg, Qt::WindowFlags f)
  : Superclass(parentWdg, f)
  , DefaultCursor(QCursor(Qt::ArrowCursor))
{
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
  iren->Initialize();

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
  if (this->RenderWindow)
  {
    this->RenderWindow->SetCustomSurfaceDescriptor(nullptr);
    this->RenderWindow->Finalize();
  }
#ifdef __APPLE__
  // Release the Metal layer if we created one
  if (this->MetalLayer)
  {
    // Release using Objective-C runtime
    ((void (*)(id, SEL))objc_msgSend)((id)this->MetalLayer, sel_registerName("release"));
    this->MetalLayer = nullptr;
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

  this->PlatformSurfaceDescriptor = {};
  this->PlatformSurfaceDescriptor.hwnd = hwnd;
  this->PlatformSurfaceDescriptor.hinstance = hinstance;

  this->SurfaceDescriptor = {};
  this->SurfaceDescriptor.label = "VTK Qt WebGPU Widget Surface";
  this->SurfaceDescriptor.nextInChain = &this->PlatformSurfaceDescriptor;

#elif defined(__APPLE__)
  // macOS: Create a Metal layer and get it from the view
  // We need to create a CAMetalLayer and set it on the NSView
  void* nsView = reinterpret_cast<void*>(this->winId());

  // Create CAMetalLayer using Objective-C runtime
  Class metalLayerClass = objc_getClass("CAMetalLayer");
  if (metalLayerClass)
  {
    this->MetalLayer =
      ((id(*)(Class, SEL))objc_msgSend)(metalLayerClass, sel_registerName("layer"));
    if (this->MetalLayer)
    {
      // Retain the layer
      ((void (*)(id, SEL))objc_msgSend)((id)this->MetalLayer, sel_registerName("retain"));

      // Set the layer on the view: [view setLayer:metalLayer]
      ((void (*)(id, SEL, id))objc_msgSend)(
        (id)nsView, sel_registerName("setLayer:"), (id)this->MetalLayer);

      // Set wantsLayer to YES: [view setWantsLayer:YES]
      ((void (*)(id, SEL, BOOL))objc_msgSend)((id)nsView, sel_registerName("setWantsLayer:"), YES);

      this->PlatformSurfaceDescriptor = {};
      this->PlatformSurfaceDescriptor.layer = this->MetalLayer;

      this->SurfaceDescriptor = {};
      this->SurfaceDescriptor.label = "VTK Qt WebGPU Widget Surface";
      this->SurfaceDescriptor.nextInChain = &this->PlatformSurfaceDescriptor;
    }
  }

#elif defined(VTK_USE_X)
  // X11: Use Xlib window
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  auto* nativeInterface = QGuiApplication::platformNativeInterface();
  Display* display =
    reinterpret_cast<Display*>(nativeInterface->nativeResourceForWindow("display", nullptr));
#else
  Display* display = QX11Info::display();
#endif
  Window window = static_cast<Window>(this->winId());

  this->PlatformSurfaceDescriptor = {};
  this->PlatformSurfaceDescriptor.display = display;
  this->PlatformSurfaceDescriptor.window = window;

  this->SurfaceDescriptor = {};
  this->SurfaceDescriptor.label = "VTK Qt WebGPU Widget Surface";
  this->SurfaceDescriptor.nextInChain = &this->PlatformSurfaceDescriptor;

#elif defined(VTK_USE_Wayland)
  // Wayland surface
  // Note: Qt5/6 may not directly expose Wayland surface, this is a placeholder
  qWarning() << "Wayland support for QVTKWebGPUWidget is not yet fully implemented";

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

  if (!this->Initialized)
  {
    this->initializeWebGPU();
  }

  if (this->RenderWindow && this->Initialized)
  {
    this->RenderWindow->Render();
  }
}

//------------------------------------------------------------------------------
bool QVTKWebGPUWidget::event(QEvent* evt)
{
  if (this->InteractorAdapter && this->RenderWindow)
  {
    if (this->InteractorAdapter->ProcessEvent(evt, this->RenderWindow->GetInteractor()))
    {
      return true;
    }
  }
  return this->Superclass::event(evt);
}

VTK_ABI_NAMESPACE_END
