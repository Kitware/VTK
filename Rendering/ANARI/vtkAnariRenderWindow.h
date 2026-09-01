// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkAnariRenderWindow
 * @brief Create a render window context for ANARI.
 *
 * vtkAnariRenderWindow is a vtkRenderWindow in the ANARI context.
 * This allows to use ANARI for rendering a 3D scene in VTK.
 * @warning the class only supports offscreen rendering. This render window can be used
 * with a vtkWindowToImageFilter to convert the rendered scene to an image.
 *
 * @note vtkAnariPass is still available for onscreen rendering. It however still requires OpenGL
 * dependency.
 * @sa vtkAnariPass
 */

#ifndef vtkAnariRenderWindow_h
#define vtkAnariRenderWindow_h

#include "vtkRenderWindow.h"
#include "vtkRenderingAnariModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkAnariSceneGraph;
class vtkAnariDevice;
class vtkAnariRenderer;
class vtkAnariViewNodeFactory;
class vtkOverrideAttribute;

class VTKRENDERINGANARI_EXPORT vtkAnariRenderWindow : public vtkRenderWindow
{
public:
  static vtkAnariRenderWindow* New();
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkAnariRenderWindow, vtkRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return the AnariDevice contained by the render window.
   */
  vtkGetObjectMacro(AnariDevice, vtkAnariDevice);

  /**
   * Return the AnariRenderer contained by the render window.
   * vtkAnariRenderer is the internal renderer for parameter management.
   */
  vtkGetObjectMacro(AnariRenderer, vtkAnariRenderer);

  /**
   * @name Pixel query functions
   *
   * Return a pixel buffer containing the pixel data in the region delimited by [x1,y1] to [x2,y2].
   * @warning It is to the caller responsibility to delete the returned buffer after usage.
   * - The `unsigned char*` return overrides return the pointer to the pixel data. If the color
   * buffer query fails in any way, it returns a nullptr.
   * - The `int` return override returns VTK_OK if the pixel data were fetched properly, otherwise
   * it returns VTK_ERROR.
   */
  ///@{

  /**
   * @brief Functions that query RGB pixel image.
   */
  unsigned char* GetPixelData(
    int x1, int y1, int x2, int y2, int vtkNotUsed(front), int vtkNotUsed(right) = 0) override;
  int GetPixelData(int x1, int y1, int x2, int y2, int vtkNotUsed(front),
    vtkUnsignedCharArray* data, int vtkNotUsed(right) = 0) override;

  /**
   * @brief Functions that query RGBA pixel image.
   */
  unsigned char* GetRGBACharPixelData(
    int x1, int y1, int x2, int y2, int vtkNotUsed(front), int vtkNotUsed(right) = 0) override;
  int GetRGBACharPixelData(int x1, int y1, int x2, int y2, int vtkNotUsed(front),
    vtkUnsignedCharArray* data, int vtkNotUsed(right) = 0) override;
  ///@}

protected:
  /**
   * As the render window only supports offscreen rendering for the moment, the constructor calls
   * OffscreenRenderingOn() to enforce it.
   */
  vtkAnariRenderWindow();
  ~vtkAnariRenderWindow() override = default;

  /**
   * Called to trigger the rendering to ANARI.
   */
  void DoStereoRender() override;

private:
  vtkAnariRenderWindow(const vtkAnariRenderWindow&) = delete;
  void operator=(const vtkAnariRenderWindow&) = delete;

  ///@{
  /**
   * Return an image buffer containing a subset of the source color buffer coming from the
   * AnariSceneGraph.
   * If the given dimensions are too big, it returns a nullptr.
   * If the success boolean is provided, it will be set to false if the sampling failed, otherwise
   * it will be set to true.
   * If a data array is provided, the function returns VTK_OK and fills the data array if the
   * sampling was successful, otherwise it return VTK_ERROR.
   */
  unsigned char* SampleSourceColorBuffer(int x1, int y1, int x2, int y2, int channelCount);
  unsigned char* SampleSourceColorBuffer(
    int x1, int y1, int x2, int y2, int channelCount, bool& success);
  int SampleSourceColorBuffer(
    int x1, int y1, int x2, int y2, int channelCount, vtkUnsignedCharArray* data);
  ///@}

  /**
   * Return RGBA pixels at the given image coordinates x and y.
   * If the source color buffer is empty, it returns nullptr.
   */
  const unsigned char* SampleSourceColorBuffer(int x, int y) const;

  ///@{
  /**
   * Return the current width or height from the AnariSceneGraph.
   */
  int GetWidth() const;
  int GetHeight() const;
  ///@}

  vtkSmartPointer<vtkAnariSceneGraph> AnariSceneGraph;

  vtkNew<vtkAnariDevice> AnariDevice;
  vtkNew<vtkAnariRenderer> AnariRenderer;

  vtkNew<vtkAnariViewNodeFactory> AnariFactory;
};

#define vtkAnariRenderWindow_OVERRIDE_ATTRIBUTES vtkAnariRenderWindow::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END

#endif // vtkAnariRenderWindow_h
