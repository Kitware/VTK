// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkStandardRenderView
 * @brief   A render view with convenient defaults for general-purpose rendering.
 *
 * vtkStandardRenderView extends vtkRenderViewBase with a trackball camera
 * interaction style, sensible default window size, and convenience methods
 * for background colors and starting the event loop.
 *
 * Users create vtkSurfaceRepresentation or vtkVolumeRepresentation objects,
 * configure them, and add them to this view.  Call Start() to enter the
 * interactive event loop.
 *
 * @par Example usage:
 * @code
 * auto view = vtkNew<vtkStandardRenderView>();
 * auto rep = vtkNew<vtkSurfaceRepresentation>();
 * rep->SetInputConnection(source->GetOutputPort());
 * rep->SetColor(0.8, 0.2, 0.2);
 * view->AddRepresentation(rep);
 * view->ResetCamera();
 * view->Start();
 * @endcode
 *
 * @sa vtkRenderViewBase vtkSurfaceRepresentation vtkVolumeRepresentation
 */

#ifndef vtkStandardRenderView_h
#define vtkStandardRenderView_h

#include "vtkNew.h" // For ivars
#include "vtkRenderViewBase.h"
#include "vtkSmartPointer.h"         // For ivars
#include "vtkViewsRenderingModule.h" // For export macro
#include "vtkWrappingHints.h"        // For VTK_MARSHALEXCLUDE

VTK_ABI_NAMESPACE_BEGIN
class vtkHardwareSelector;
class vtkInteractorObserver;
class vtkInteractorStyleRubberBand3D;
class vtkInteractorStyleTrackballCamera;
class vtkLight;
class vtkLightKit;
class vtkOrientationMarkerWidget;
class vtkSelection;

class VTKVIEWSRENDERING_EXPORT vtkStandardRenderView : public vtkRenderViewBase
{
public:
  static vtkStandardRenderView* New();
  vtkTypeMacro(vtkStandardRenderView, vtkRenderViewBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The view stores its configuration on the renderer, render window, light kit
   * and orientation marker widget rather than in its own ivars, so the modified
   * time reported here is the latest of this object's own and theirs.  Note
   * that this makes the view report scene changes too, since adding or removing
   * a representation modifies the renderer.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/Get the background color of the renderer.  SetBackground sets the
   * bottom color of the gradient, SetBackground2 the top color.
   */
  void SetBackground(double r, double g, double b);
  double* GetBackground() VTK_SIZEHINT(3);
  void SetBackground2(double r, double g, double b);
  double* GetBackground2() VTK_SIZEHINT(3);
  void SetGradientBackground(bool val);
  bool GetGradientBackground();
  ///@}

  ///@{
  /**
   * Set/Get the render window size and title.
   */
  void SetWindowSize(int w, int h);
  int* GetWindowSize() VTK_SIZEHINT(2);
  void SetWindowTitle(const char* title);
  const char* GetWindowTitle();
  ///@}

  ///@{
  /**
   * Show or hide the orientation axes marker in the lower-left corner
   * of the render window.  The marker shows the current camera
   * orientation as an interactive XYZ axes triad.  Default is on.
   */
  void SetOrientationAxesVisibility(bool val);
  bool GetOrientationAxesVisibility();
  ///@}

  ///@{
  /**
   * Set whether the orientation axes marker can be interactively
   * moved and resized by the user.  Default is off.
   */
  void SetOrientationAxesInteractive(bool val);
  bool GetOrientationAxesInteractive();
  ///@}

  /**
   * Provide access to the orientation marker widget for advanced usage.
   */
  vtkOrientationMarkerWidget* GetOrientationMarkerWidget();

  ///@{
  /**
   * Enable or disable the light kit.  When enabled, the renderer's
   * existing lights are replaced by a professional 5-light rig
   * (key, fill, headlight, and two back lights) managed by vtkLightKit.
   * When disabled, the light kit lights are removed and the renderer
   * reverts to its default single headlight.
   * Default is off (single headlight).
   */
  void SetUseLightKit(bool val);
  bool GetUseLightKit();
  ///@}

  ///@{
  /**
   * Light kit parameters (forwarded to the internal vtkLightKit).
   * These only take effect when the light kit is enabled.
   *
   * KeyLightIntensity controls the overall brightness (default 0.75).
   * KeyToFillRatio, KeyToHeadRatio, and KeyToBackRatio control the
   * relative intensity of the fill, headlight, and back lights
   * compared to the key light (defaults 3.0, 3.0, 3.5).
   *
   * Warmth values control color temperature on a 0-1 scale where
   * 0 is cold blue, 0.5 is neutral white, and 1 is warm sunset
   * (defaults: key 0.6, fill 0.4, head 0.5, back 0.5).
   *
   * KeyLightAngle and FillLightAngle position the key and fill
   * lights by elevation and azimuth in degrees
   * (defaults: key 50/-10, fill -75/-10).
   * BackLightAngle positions both back lights symmetrically.
   *
   * MaintainLuminance compensates for perceived brightness
   * differences across color temperatures (default off).
   */
  void SetKeyLightIntensity(double val);
  double GetKeyLightIntensity();
  void SetKeyToFillRatio(double val);
  double GetKeyToFillRatio();
  void SetKeyToHeadRatio(double val);
  double GetKeyToHeadRatio();
  void SetKeyToBackRatio(double val);
  double GetKeyToBackRatio();
  void SetKeyLightWarmth(double val);
  double GetKeyLightWarmth();
  void SetFillLightWarmth(double val);
  double GetFillLightWarmth();
  void SetHeadLightWarmth(double val);
  double GetHeadLightWarmth();
  void SetBackLightWarmth(double val);
  double GetBackLightWarmth();
  void SetKeyLightAngle(double elevation, double azimuth);
  double* GetKeyLightAngle() VTK_SIZEHINT(2);
  void SetFillLightAngle(double elevation, double azimuth);
  double* GetFillLightAngle() VTK_SIZEHINT(2);
  void SetBackLightAngle(double elevation, double azimuth);
  double* GetBackLightAngle() VTK_SIZEHINT(2);
  void SetMaintainLuminance(bool val);
  bool GetMaintainLuminance();
  ///@}

  /**
   * Provide access to the internal light kit for advanced usage.
   */
  vtkLightKit* GetLightKit();

  ///@{
  /**
   * Add or remove individual lights from the renderer.
   * These methods work regardless of whether the light kit is enabled.
   * When a light kit is active, manually added lights supplement
   * the light kit lights.
   */
  void AddLight(vtkLight* light);
  void RemoveLight(vtkLight* light);
  void RemoveAllLights();
  ///@}

  ///@{
  /**
   * Interaction mode constants.
   * INTERACTION_MODE_3D uses vtkInteractorStyleTrackballCamera (default).
   * INTERACTION_MODE_SELECTION uses vtkInteractorStyleRubberBand3D for
   * rubber-band selection.  Mode switching is programmatic; applications
   * typically expose it through a toolbar or keyboard shortcut.
   * INTERACTION_MODE_CUSTOM reports that a style supplied through
   * SetInteractorStyle() is installed; it cannot be selected directly.
   *
   * The view owns one instance of each built-in style and switches between
   * them, so any configuration applied to a style survives a mode change.
   */
  enum
  {
    INTERACTION_MODE_3D = 0,
    INTERACTION_MODE_SELECTION = 1,
    INTERACTION_MODE_CUSTOM = 2
  };
  void SetInteractionMode(int mode);
  int GetInteractionMode();
  void SetInteractionModeTo3D() { this->SetInteractionMode(INTERACTION_MODE_3D); }
  void SetInteractionModeToSelection() { this->SetInteractionMode(INTERACTION_MODE_SELECTION); }
  ///@}

  ///@{
  /**
   * Install an interactor style of your own, for interaction the built-in
   * modes do not cover -- a 2D or terrain camera, or a
   * vtkInteractorStyleManipulator configured with your own button bindings.
   * The view holds a reference to the style and reports
   * INTERACTION_MODE_CUSTOM until a built-in mode is selected again, at which
   * point the custom style is remembered and can be restored by setting it
   * once more.  Passing nullptr returns to INTERACTION_MODE_3D.
   *
   * To drive selection from a custom style, call SelectRegion() from its event
   * handling.  The view does not interpret events from a style it does not
   * know.
   *
   * GetInteractorStyle() returns the style currently installed on the
   * interactor, built-in or custom.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED)
  void SetInteractorStyle(vtkInteractorObserver* style);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED)
  vtkInteractorObserver* GetInteractorStyle();
  ///@}

  ///@{
  /**
   * Selection mode constants.
   * SELECTION_MODE_SURFACE uses hardware picking (vtkHardwareSelector)
   * to select visible cells/points on the surface.
   * SELECTION_MODE_FRUSTUM selects all cells/points within the
   * viewing frustum defined by the rubber-band rectangle.
   */
  enum
  {
    SELECTION_MODE_SURFACE = 0,
    SELECTION_MODE_FRUSTUM = 1
  };
  void SetSelectionMode(int mode);
  int GetSelectionMode();
  void SetSelectionModeToSurface() { this->SetSelectionMode(SELECTION_MODE_SURFACE); }
  void SetSelectionModeToFrustum() { this->SetSelectionMode(SELECTION_MODE_FRUSTUM); }
  ///@}

  ///@{
  /**
   * Field association for selection.
   * Determines whether cells or points are selected.
   * Uses vtkDataObject::FIELD_ASSOCIATION_CELLS (default) or
   * vtkDataObject::FIELD_ASSOCIATION_POINTS.
   */
  void SetSelectionFieldAssociation(int assoc);
  int GetSelectionFieldAssociation();
  void SelectCells();
  void SelectPoints();
  ///@}

  /**
   * Select the screen-space region bounded by the two corner points, exactly
   * as an interactive rubber-band selection would.  The corners are given in
   * display coordinates and may be in any order; the region is normalized and
   * clamped to the renderer.  A degenerate region (a click) is expanded
   * slightly so that it still picks.
   *
   * The selection honors the current selection mode and field association, is
   * applied to every representation, becomes the current selection, and fires
   * SelectionChangedEvent.  When extend is true it adds to the existing
   * selection instead of replacing it.
   *
   * This is the entry point the built-in rubber-band interaction uses, and the
   * supported way for application code or a custom interactor style to drive
   * selection without synthesizing interactor events.
   */
  void SelectRegion(int x0, int y0, int x1, int y1, bool extend = false);

  /**
   * Clear the current selection on all representations.
   */
  void ClearSelection();

  /**
   * Get the current selection.  This is the combined selection
   * produced by the last interactive or programmatic selection
   * operation, containing nodes for all picked actors.
   * Returns nullptr if no selection has been made.
   *
   * After a selection is made, the view fires SelectionChangedEvent
   * with the vtkSelection pointer as call data.  Observers can
   * retrieve the selection from this method or from the call data.
   */
  vtkSelection* GetCurrentSelection();

  /**
   * Start the interactor event loop.  Calls Render() first.
   */
  void Start();

  /**
   * Override to auto-reset camera on first render.
   */
  void Render() override;

protected:
  vtkStandardRenderView();
  ~vtkStandardRenderView() override;

  void ProcessEvents(vtkObject* caller, unsigned long eventId, void* callData) override;

private:
  vtkStandardRenderView(const vtkStandardRenderView&) = delete;
  void operator=(const vtkStandardRenderView&) = delete;

  /**
   * Build a selection for the given display-space region, which must already
   * be normalized and clamped to the renderer.
   */
  void GenerateSelection(const int region[4], vtkSelection* sel);

  vtkNew<vtkOrientationMarkerWidget> OrientationWidget;
  vtkNew<vtkLightKit> LightKit;
  vtkNew<vtkHardwareSelector> HardwareSelector;
  vtkSmartPointer<vtkSelection> CurrentSelection;

  // One instance of each built-in style, so switching modes preserves whatever
  // the application configured on them.
  vtkSmartPointer<vtkInteractorStyleTrackballCamera> TrackballStyle;
  vtkSmartPointer<vtkInteractorStyleRubberBand3D> RubberBandStyle;
  vtkSmartPointer<vtkInteractorObserver> CustomStyle;
  bool UseLightKitFlag;
  bool FirstRender;
  int InteractionMode;
  int SelectionMode;
  int FieldAssociation;
};

VTK_ABI_NAMESPACE_END
#endif
