// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLightingMapPass
 * @brief   TO DO
 *
 * Renders lighting information directly instead of final shaded colors.
 * The information keys allow the selection of either normal rendering or
 * luminance. For normals, the (nx, ny, nz) tuple are rendered directly into
 * the (r,g,b) fragment. For luminance, the diffuse and specular intensities are
 * rendered into the red and green channels, respectively. The blue channel is
 * zero. For both luminances and normals, the alpha channel is set to 1.0 if
 * present.
 *
 * @sa
 * vtkRenderPass vtkDefaultPass
 */

#ifndef vtkLightingMapPass_h
#define vtkLightingMapPass_h

#include "vtkDefaultPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationIntegerKey;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkLightingMapPass : public vtkDefaultPass
{
public:
  static vtkLightingMapPass* New();
  vtkTypeMacro(vtkLightingMapPass, vtkDefaultPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the type of lighting render to perform
   */
  enum RenderMode
  {
    LUMINANCE,
    NORMALS
  };
  vtkSetMacro(RenderType, RenderMode);
  vtkGetMacro(RenderType, RenderMode);
  ///@}

  /**
   * If this key exists on the PropertyKeys of a prop, the active scalar array
   * on the prop will be rendered as its color. This key is mutually exclusive
   * with the RENDER_LUMINANCE key.
   */
  static vtkInformationIntegerKey* RENDER_LUMINANCE();

  /**
   * if this key exists on the ProperyKeys of a prop, the active vector array on
   * the prop will be rendered as its color. This key is mutually exclusive with
   * the RENDER_LUMINANCE key.
   */
  static vtkInformationIntegerKey* RENDER_NORMALS();

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState* s) override;

protected:
  /**
   * Default constructor.
   */
  vtkLightingMapPass();

  /**
   * Destructor.
   */
  ~vtkLightingMapPass() override;

  /**
   * Opaque pass with key checking.
   * \pre s_exists: s!=0
   */
  void RenderOpaqueGeometry(const vtkRenderState* s) override;

private:
  vtkLightingMapPass(const vtkLightingMapPass&) = delete;
  void operator=(const vtkLightingMapPass&) = delete;

  RenderMode RenderType;
};

VTK_ABI_NAMESPACE_END
#endif
