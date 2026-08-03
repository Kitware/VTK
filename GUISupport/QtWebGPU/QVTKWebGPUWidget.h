// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class QVTKWebGPUWidget
 * @brief QWidget subclass to house a vtkWebGPURenderWindow in a Qt application.
 *
 * QVTKWebGPUWidget extends QWidget to make it work with a vtkWebGPURenderWindow.
 * It creates a WebGPU surface from the native Qt window handle and sets it on
 * the render window using a custom surface descriptor.
 *
 * A typical usage for QVTKWebGPUWidget is as follows:
 * @code{.cpp}
 *
 *  QPointer<QVTKWebGPUWidget> widget = new QVTKWebGPUWidget(...);
 *
 *  // Get the render window and add renderers, actors, etc.
 *  vtkWebGPURenderWindow* renWin = widget->renderWindow();
 *  vtkNew<vtkRenderer> renderer;
 *  renWin->AddRenderer(renderer);
 *
 * @endcode
 *
 * @sa vtkWebGPURenderWindow
 */

#ifndef QVTKWebGPUWidget_h
#define QVTKWebGPUWidget_h

#include <QWidget>
#include <memory> // for std::unique_ptr

#include "QVTKInteractor.h"              // needed for QVTKInteractor
#include "vtkGUISupportQtWebGPUModule.h" // for export macro
#include "vtkSmartPointer.h"             // needed for vtkSmartPointer

#include "vtk_wgpu.h" // for webgpu

VTK_ABI_NAMESPACE_BEGIN
class QVTKInteractor;
class QVTKInteractorAdapter;
class vtkWebGPURenderWindow;

class VTKGUISUPPORTQTWEBGPU_EXPORT QVTKWebGPUWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  QVTKWebGPUWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  ~QVTKWebGPUWidget() override;

  /**
   * Returns the render window that is being shown in this widget.
   */
  vtkWebGPURenderWindow* renderWindow() const;

  /**
   * Get the QVTKInteractor that was either created by default or set by the user.
   */
  QVTKInteractor* interactor() const;

  ///@{
  /**
   * Enable or disable support for HiDPI displays. When enabled, this enabled
   * DPI scaling i.e. `vtkWindow::SetDPI` will be called with a DPI value scaled
   * by the device pixel ratio every time the widget is resized. The unscaled
   * DPI value can be specified by using `setUnscaledDPI`.
   */
  void setEnableHiDPI(bool enable);
  bool enableHiDPI() const { return this->EnableHiDPI; }
  ///@}

  ///@{
  /**
   * Set/Get unscaled DPI value. Defaults to 72, which is also the default value
   * in vtkWindow.
   */
  void setUnscaledDPI(int dpi);
  int unscaledDPI() const { return this->UnscaledDPI; }
  ///@}

  ///@{
  /**
   * Set/Get a custom device pixel ratio to use to map Qt sizes to VTK (or
   * WebGPU) sizes. Thus, when the QWidget is resized, it called
   * `vtkRenderWindow::SetSize` on the internal vtkRenderWindow after
   * multiplying the QWidget's size by this scale factor.
   *
   * By default, this is set to 0. Which means that `devicePixelRatio` obtained
   * from Qt will be used. Set this to a number greater than 0 to override this
   * behaviour and use the custom scale factor instead.
   *
   * `effectiveDevicePixelRatio` can be used to obtain the device-pixel-ratio
   * that will be used given the value for customDevicePixelRatio.
   */
  void setCustomDevicePixelRatio(double cdpr);
  double customDevicePixelRatio() const { return this->CustomDevicePixelRatio; }
  double effectiveDevicePixelRatio() const;
  ///@}

  ///@{
  /**
   * Set/get the default cursor to use for this widget.
   */
  void setDefaultCursor(const QCursor& cursor);
  const QCursor& defaultCursor() const { return this->DefaultCursor; }
  ///@}

protected:
  void resizeEvent(QResizeEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  bool event(QEvent* evt) override;

  /**
   * Initialize the WebGPU render window with the native window handle.
   */
  void initializeWebGPU();

  /**
   * Create the WebGPU surface descriptor from the native window handle.
   */
  void createSurfaceDescriptor();

  /**
   * Update the render window size based on the widget size.
   */
  void updateSize();

  vtkSmartPointer<vtkWebGPURenderWindow> RenderWindow;
  std::unique_ptr<QVTKInteractorAdapter> InteractorAdapter;

private:
  Q_DISABLE_COPY(QVTKWebGPUWidget);

  bool Initialized = false;
  bool InPaint = false;
  bool EnableHiDPI = true;
  int UnscaledDPI = 72;
  double CustomDevicePixelRatio = 0.0;
  QCursor DefaultCursor;

  // Platform-specific surface descriptor storage
#ifdef _WIN32
  wgpu::SurfaceSourceWindowsHWND PlatformSurfaceDescriptor;
#elif defined(__APPLE__)
  wgpu::SurfaceSourceMetalLayer PlatformSurfaceDescriptor;
  void* MetalLayer = nullptr;
#elif defined(VTK_USE_Wayland)
  wgpu::SurfaceSourceWaylandSurface PlatformSurfaceDescriptor;
#elif defined(VTK_USE_X)
  wgpu::SurfaceSourceXlibWindow PlatformSurfaceDescriptor;
#endif
  wgpu::SurfaceDescriptor SurfaceDescriptor;
};

VTK_ABI_NAMESPACE_END
#endif
