// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTexturedActor2D
 * @brief   actor that draws 2D data with texture support
 *
 * vtkTexturedActor2D is an Actor2D which has additional support for
 * textures, just like vtkActor. To use textures, the geometry must have
 * texture coordinates, and the texture must be set with SetTexture().
 *
 * @sa
 * vtkActor2D vtkProp vtkMapper2D vtkProperty2D
 */

#ifndef vtkTexturedActor2D_h
#define vtkTexturedActor2D_h

#include "vtkActor2D.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkProp;
class vtkTexture;
class vtkViewport;
class vtkWindow;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkTexturedActor2D : public vtkActor2D
{
public:
  static vtkTexturedActor2D* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkTexturedActor2D, vtkActor2D);

  ///@{
  /**
   * Set/Get the texture object to control rendering texture maps.  This will
   * be a vtkTexture object. An actor does not need to have an associated
   * texture map and multiple actors can share one texture.
   */
  virtual void SetTexture(vtkTexture* texture);
  vtkGetObjectMacro(Texture, vtkTexture);
  ///@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow* win) override;

  ///@{
  /**
   * Support the standard render methods.
   */
  int RenderOverlay(vtkViewport* viewport) override;
  int RenderOpaqueGeometry(vtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) override;
  ///@}

  /**
   * Return this object's modified time.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Shallow copy of this vtkTexturedActor2D. Overrides vtkActor2D method.
   */
  void ShallowCopy(vtkProp* prop) override;

protected:
  vtkTexturedActor2D();
  ~vtkTexturedActor2D() override;

  vtkTexture* Texture;

private:
  vtkTexturedActor2D(const vtkTexturedActor2D&) = delete;
  void operator=(const vtkTexturedActor2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
