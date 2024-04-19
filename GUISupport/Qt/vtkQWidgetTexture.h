// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkQWidgetTexture_h
#define vtkQWidgetTexture_h

#include "vtkGUISupportQtModule.h" // For export macro
#include "vtkTextureObject.h"
#include <array>      // for ivar
#include <functional> // for ivar

class QGraphicsScene;
class QWidget;

VTK_ABI_NAMESPACE_BEGIN

/**
 * @class vtkQWidgetTexture
 * @brief Allows a QWidget to be used as a texture in VTK with OpenGL
 *
 * This class works by rendering the QWidget into a Framebuffer
 * and then sending the OpenGL texture handle to VTK for rendering.
 */
class VTKGUISUPPORTQT_EXPORT vtkQWidgetTexture : public vtkTextureObject
{
public:
  static vtkQWidgetTexture* New();
  vtkTypeMacro(vtkQWidgetTexture, vtkTextureObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the QWidget that this TextureObject will render/use
   */
  void SetWidget(QWidget* w);
  QWidget* GetWidget() { return this->Widget; }
  ///@}

  /**
   * get the QScene used for rendering, this is where events will
   * be forwarded to.
   */
  QGraphicsScene* GetScene() { return this->Scene; }

  /**
   * Activate and Bind the texture. Overloaded to handle the opengl related
   * setup at the same time. as We know the context will be active then.
   */
  void Activate() override;

  /**
   * Free resources
   */
  void ReleaseGraphicsResources(vtkWindow* win) override;

protected:
  vtkQWidgetTexture();
  ~vtkQWidgetTexture() override;

  QGraphicsScene* Scene;
  QWidget* Widget;

  // method called when the widget needs repainting
  std::function<void()> RedrawMethod;

  // internal method to setup the scene/framebuffer/etc
  void AllocateFromWidget();

  unsigned char* ImageBuffer;
  std::array<int, 2> ImageBufferDimensions;

private:
  vtkQWidgetTexture(const vtkQWidgetTexture&) = delete;
  void operator=(const vtkQWidgetTexture&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
