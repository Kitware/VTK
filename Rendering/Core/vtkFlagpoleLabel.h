// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkFlagpoleLabel
 * @brief Renders a flagpole (line) with a label at the top that faces the camera
 *
 * This class draws a line from the base to the top of the flagpole. It then
 * places a text annotation at the top, centered horizontally. The text is
 * always oriented with the flagpole but will rotate aroundthe flagpole to
 * face the camera.
 */

#ifndef vtkFlagpoleLabel_h
#define vtkFlagpoleLabel_h

#include "vtkActor.h"
#include "vtkNew.h"                 // For.... vtkNew!
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkSmartPointer.h"        // For.... vtkSmartPointer!

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkImageData;
class vtkLineSource;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkTextProperty;
class vtkTextRenderer;

class VTKRENDERINGCORE_EXPORT vtkFlagpoleLabel : public vtkActor
{
public:
  static vtkFlagpoleLabel* New();
  vtkTypeMacro(vtkFlagpoleLabel, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The UTF-8 encoded string to display.
   * @{
   */
  void SetInput(const char* in);
  vtkGetStringMacro(Input);
  /** @} */

  /**
   * The vtkTextProperty object that controls the rendered text.
   * @{
   */
  void SetTextProperty(vtkTextProperty* tprop);
  vtkGetObjectMacro(TextProperty, vtkTextProperty);
  /** @} */

  /**
   * Force the actor to render during the opaque or translucent pass.
   * @{
   */
  void SetForceOpaque(bool opaque) override;
  bool GetForceOpaque() VTK_FUTURE_CONST override;
  void ForceOpaqueOn() override;
  void ForceOpaqueOff() override;
  void SetForceTranslucent(bool trans) override;
  bool GetForceTranslucent() VTK_FUTURE_CONST override;
  void ForceTranslucentOn() override;
  void ForceTranslucentOff() override;
  /**@}*/

  /**
   * Defers to internal actor.
   */
  vtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Check/update geometry/texture in opaque pass, since it only happens once.
   */
  int RenderOpaqueGeometry(vtkViewport* vp) override;

  /**
   * Just render in translucent pass, since it can execute multiple times
   * (depth peeling, for instance).
   */
  int RenderTranslucentPolygonalGeometry(vtkViewport* vp) override;

  void ReleaseGraphicsResources(vtkWindow* win) override;
  double* GetBounds() override;
  using Superclass::GetBounds;

  /**
   * Set/Get the world coordinate position of the base
   */
  vtkGetVector3Macro(BasePosition, double);
  void SetBasePosition(double x, double y, double z);

  /**
   * Set/Get the world coordinate position of the top
   */
  vtkGetVector3Macro(TopPosition, double);
  void SetTopPosition(double x, double y, double z);

  /**
   * Set/Get the size of the flag. 1.0 is the default size
   * which corresponds to a preset texels/window value. Adjust this
   * to increase or decrease the default size.
   */
  vtkGetMacro(FlagSize, double);
  vtkSetMacro(FlagSize, double);

protected:
  vtkFlagpoleLabel();
  ~vtkFlagpoleLabel() override;

  bool InputIsValid();

  void UpdateInternals(vtkRenderer* ren);

  bool TextureIsStale(vtkRenderer* ren);
  void GenerateTexture(vtkRenderer* ren);

  bool QuadIsStale(vtkRenderer* ren);
  void GenerateQuad(vtkRenderer* ren);

  // Used by the opaque pass to tell the translucent pass not to render.
  void Invalidate();
  bool IsValid();

  // Used to sync the internal actor's state.
  void PreRender();

  // Text specification:
  char* Input;
  vtkTextProperty* TextProperty;

  // Cached metadata to determine if things need rebuildin'
  int RenderedDPI;
  vtkTimeStamp InputMTime;

  // We cache this so we can recompute the bounds between renders, if needed.
  vtkSmartPointer<vtkRenderer> RenderedRenderer;

  // Rendering stuffies
  vtkNew<vtkTextRenderer> TextRenderer;
  vtkNew<vtkImageData> Image;
  vtkNew<vtkPolyData> Quad;
  vtkNew<vtkPolyDataMapper> QuadMapper;
  vtkNew<vtkActor> QuadActor;

  vtkNew<vtkPolyDataMapper> PoleMapper;
  vtkNew<vtkLineSource> LineSource;
  vtkNew<vtkActor> PoleActor;

  double TopPosition[3];
  double BasePosition[3];
  double FlagSize;

private:
  vtkFlagpoleLabel(const vtkFlagpoleLabel&) = delete;
  void operator=(const vtkFlagpoleLabel&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkFlagpoleLabel_h
