/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBillboardTextActor3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkBillboardTextActor3D
 * @brief Renders pixel-aligned text, facing the camera, anchored at a 3D point.
 */

#ifndef vtkBillboardTextActor3D_h
#define vtkBillboardTextActor3D_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkProp3D.h"
#include "vtkNew.h" // For.... vtkNew!

class vtkActor;
class vtkImageData;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkTextProperty;
class vtkTextRenderer;
class vtkTexture;

class VTKRENDERINGCORE_EXPORT vtkBillboardTextActor3D: public vtkProp3D
{
public:
  static vtkBillboardTextActor3D* New();
  vtkTypeMacro(vtkBillboardTextActor3D, vtkProp3D)
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * The UTF-8 encoded string to display.
   * @{
   */
  void SetInput(const char *in);
  vtkGetStringMacro(Input)
  /** @} */

  /**
   * Can be used to set a fixed offset from the anchor point.
   * Use display coordinates.
   * @{
   */
  vtkGetVector2Macro(DisplayOffset, int)
  vtkSetVector2Macro(DisplayOffset, int)
  /** @} */

  /**
   * The vtkTextProperty object that controls the rendered text.
   * @{
   */
  void SetTextProperty(vtkTextProperty *tprop);
  vtkGetObjectMacro(TextProperty, vtkTextProperty)
  /** @} */

  /**
   * Force the actor to render during the opaque or translucent pass.
   * @{
   */
  virtual void SetForceOpaque(bool opaque);
  virtual bool GetForceOpaque();
  virtual void ForceOpaqueOn();
  virtual void ForceOpaqueOff();
  virtual void SetForceTranslucent(bool trans);
  virtual bool GetForceTranslucent();
  virtual void ForceTranslucentOn();
  virtual void ForceTranslucentOff();
  /**@}*/

  /**
   * Defers to internal actor.
   */
  int HasTranslucentPolygonalGeometry() VTK_OVERRIDE;

  /**
   * Check/update geometry/texture in opaque pass, since it only happens once.
   */
  int RenderOpaqueGeometry(vtkViewport *vp) VTK_OVERRIDE;

  /**
   * Just render in translucent pass, since it can execute multiple times
   * (depth peeling, for instance).
   */
  int RenderTranslucentPolygonalGeometry(vtkViewport *vp) VTK_OVERRIDE;

  void ReleaseGraphicsResources(vtkWindow *win) VTK_OVERRIDE;
  double *GetBounds() VTK_OVERRIDE;
  using Superclass::GetBounds;

  /**
   * Returns the anchor position in display coordinates, with depth in NDC.
   * Valid after calling RenderOpaqueGeometry.
   */
  vtkGetVector3Macro(AnchorDC, double)

protected:
  vtkBillboardTextActor3D();
  ~vtkBillboardTextActor3D() VTK_OVERRIDE;

  bool InputIsValid();

  bool TextureIsStale(vtkRenderer *ren);
  void GenerateTexture(vtkRenderer *ren);

  bool QuadIsStale(vtkRenderer *ren);
  void GenerateQuad(vtkRenderer *ren);

  // Used by the opaque pass to tell the translucent pass not to render.
  void Invalidate();
  bool IsValid();

  // Used to sync the internal actor's state.
  void PreRender();

  // Text specification:
  char *Input;
  vtkTextProperty *TextProperty;

  // Offset in display coordinates.
  int DisplayOffset[2];

  // Cached metadata to determine if things need rebuildin'
  int RenderedDPI;
  vtkTimeStamp InputMTime;

  // Rendering stuffies
  vtkNew<vtkTextRenderer> TextRenderer;
  vtkNew<vtkImageData> Image;
  vtkNew<vtkTexture> Texture;
  vtkNew<vtkPolyData> Quad;
  vtkNew<vtkPolyDataMapper> QuadMapper;
  vtkNew<vtkActor> QuadActor;

  // Display coordinate for anchor position. Z value is in NDC.
  // Cached for GL2PS export on OpenGL2:
  double AnchorDC[3];

private:
  vtkBillboardTextActor3D(const vtkBillboardTextActor3D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBillboardTextActor3D&) VTK_DELETE_FUNCTION;
};

#endif // vtkBillboardTextActor3D_h
