// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkScivisView
 * @brief   A ready-made rendering assembly for scientific visualization.
 *
 * Every VTK rendering program builds the same thing by hand: a renderer, a
 * render window, an interactor, an interactor style, a light, an orientation
 * marker, and the wiring between them.  vtkScivisView is that assembly, already
 * built.  A view you have just created draws something reasonable before you
 * configure anything, and it stays consistent when you replace a piece of it:
 * hand it a render window of your own and the renderers move across, while an
 * interactor style you had installed survives the change.
 *
 * @par Getting started:
 * Create representations, configure them, and add them to the view.  Start()
 * enters the interactive event loop.
 * @code
 * vtkNew<vtkScivisView> view;
 * vtkNew<vtkSurfaceRepresentation> rep;
 * rep->SetInputConnection(source->GetOutputPort());
 * rep->SetColor(0.8, 0.2, 0.2);
 * view->AddRepresentation(rep);
 * view->ResetCamera();
 * view->Start();
 * @endcode
 *
 * @par What the view itself owns:
 * Whatever is true of the scene as a whole.
 * - the background, and the render window and its title
 * - how the scene is lit: a single headlight, or a five light kit
 * - how the user interacts with it, and the orientation axes marker
 * - what is selected in it
 * - framing the camera, bringing every representation up to date, and drawing
 *
 * @par
 * Whatever is true of one thing being drawn -- its data, its geometry, its
 * color -- belongs to that representation instead.  That line is worth
 * remembering, because it is what says where any given piece of API lives.
 *
 * @par Reaching further in:
 * The view is deliberately not a mirror of the objects underneath it.  Where
 * you need more than the scene-wide API above, ask it for the object that owns
 * what you are after.
 * - GetRenderer() for lights and props of your own
 * - GetLightKit() for the light rig's intensity, warmth, angle and ratios
 * - GetOrientationMarkerWidget() for the axes marker's viewport and marker
 * - GetSelector() for how a screen region becomes a selection
 *
 * @par A family of its own:
 * This is not a vtkView, and it shows only vtkScivisRepresentations.  The
 * information visualization views and representations are a separate family
 * sharing no base class with this one: they drive their own render passes and
 * labelling, and carry annotation links and themes that mean nothing here.
 * Neither family can stand in for the other, and the type system says so rather
 * than a downcast at run time.
 *
 * @sa vtkScivisRepresentation vtkScivisDataRepresentation vtkScivisSelector
 * vtkSurfaceRepresentation vtkVolumeRepresentation
 */

#ifndef vtkScivisView_h
#define vtkScivisView_h

#include "vtkNew.h" // For ivars
#include "vtkObject.h"
#include "vtkSmartPointer.h"      // For ivars
#include "vtkViewsScivisModule.h" // For export macro
#include "vtkWrappingHints.h"     // For VTK_MARSHALEXCLUDE

VTK_ABI_NAMESPACE_BEGIN
class vtkInteractorObserver;
class vtkInteractorStyleRubberBand3D;
class vtkInteractorStyleTrackballCamera;
class vtkLight;
class vtkLightKit;
class vtkOrientationMarkerWidget;
class vtkScivisSelector;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkRenderer;
class vtkScivisRepresentation;

class VTKVIEWSSCIVIS_EXPORT vtkScivisView : public vtkObject
{
public:
  static vtkScivisView* New();
  vtkTypeMacro(vtkScivisView, vtkObject);
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
   * The representations this view shows.
   *
   * AddRepresentation() offers the view to the representation and drops it
   * again if the representation refuses, which is how a representation declines
   * a view it cannot work in.  SetRepresentation() is the same after removing
   * whatever is already there.
   */
  void AddRepresentation(vtkScivisRepresentation* rep);
  void SetRepresentation(vtkScivisRepresentation* rep);
  void RemoveRepresentation(vtkScivisRepresentation* rep);
  void RemoveAllRepresentations();
  int GetNumberOfRepresentations();
  vtkScivisRepresentation* GetRepresentation(int index = 0);
  bool IsRepresentationPresent(vtkScivisRepresentation* rep);
  ///@}

  ///@{
  /**
   * The renderer, render window and interactor this view draws with.  All three
   * are created with the view; setting one replaces it, and the others are
   * rewired to match -- the renderers move to a window given here, and an
   * interactor style already in use survives the change.  This is what an
   * application does to draw the view into a window of its own.
   */
  vtkRenderer* GetRenderer();
  void SetRenderer(vtkRenderer* renderer);
  vtkRenderWindow* GetRenderWindow();
  void SetRenderWindow(vtkRenderWindow* window);
  vtkRenderWindowInteractor* GetInteractor();
  void SetInteractor(vtkRenderWindowInteractor* interactor);
  ///@}

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

  /**
   * The orientation marker widget itself, for its viewport, its marker, and
   * whether the user can move and resize it.
   */
  vtkOrientationMarkerWidget* GetOrientationMarkerWidget();

  ///@{
  /**
   * Enable or disable the light kit: a five light rig -- key, fill, headlight
   * and two back lights -- managed by vtkLightKit.  Default is off, which
   * leaves the scene lit by a single headlight the view owns.
   *
   * Only that headlight gives way to the kit.  Lights added to the renderer,
   * which GetRenderer() returns, keep lighting the scene whether the kit is on
   * or off, and survive it being switched either way.
   */
  void SetUseLightKit(bool val);
  bool GetUseLightKit();
  ///@}

  /**
   * The light kit itself, for its intensity, warmth, angle and ratio
   * parameters.  Only has an effect while the light kit is enabled.
   *
   * Lights of your own are not part of the kit: add them to the renderer,
   * which GetRenderer() returns, with vtkRenderer::AddLight().
   */
  vtkLightKit* GetLightKit();

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

  /**
   * Selection: how a screen region becomes a selection, whether points or cells
   * come back, and what was picked last.
   *
   * The view keeps only the interaction side of selection -- which interactor
   * style is installed, and so whether a drag draws a rubber band at all.  What
   * a rubber band then does is the selector's, and application code drives it
   * the same way:
   *
   * @code
   * view->GetSelector()->SetModeToFrustum();
   * view->GetSelector()->SelectRegion(10, 10, 200, 200);
   * @endcode
   */
  vtkScivisSelector* GetSelector();

  void Start();

  /**
   * Bring every representation up to date and draw.  The camera is framed on
   * the scene the first time, and left alone after that.
   */
  virtual void Render();

  /**
   * Frame the camera on everything the representations are drawing.
   */
  virtual void ResetCamera();

  /**
   * Bring every representation up to date without drawing.
   */
  virtual void Update();

protected:
  vtkScivisView();
  ~vtkScivisView() override;

  /**
   * Called before every render, once the representations are up to date.
   */
  virtual void PrepareForRendering();

  /**
   * The observer this view listens through.  Representations are given it when
   * they are added, and anything else the view wants to hear from can be too.
   */
  vtkCommand* GetObserver();

  /**
   * Called for every event the view is observing.
   */
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId, void* callData);

  ///@{
  /**
   * Hooks for subclasses, called after a representation has been accepted or
   * before one is dropped.
   */
  virtual void AddRepresentationInternal(vtkScivisRepresentation* vtkNotUsed(rep)) {}
  virtual void RemoveRepresentationInternal(vtkScivisRepresentation* vtkNotUsed(rep)) {}
  ///@}

  vtkSmartPointer<vtkRenderer> Renderer;
  vtkSmartPointer<vtkRenderWindow> RenderWindow;

private:
  vtkScivisView(const vtkScivisView&) = delete;
  void operator=(const vtkScivisView&) = delete;

  class Command;
  friend class Command;
  Command* Observer;

  class Internals;
  Internals* Implementation;

  vtkNew<vtkOrientationMarkerWidget> OrientationWidget;
  vtkNew<vtkLightKit> LightKit;
  // The headlight that lights the scene while the light kit is off.  Owning one
  // is what keeps vtkRenderer from inventing its own -- see the constructor.
  vtkNew<vtkLight> DefaultLight;
  vtkNew<vtkScivisSelector> Selector;

  // One instance of each built-in style, so switching modes preserves whatever
  // the application configured on them.
  vtkSmartPointer<vtkInteractorStyleTrackballCamera> TrackballStyle;
  vtkSmartPointer<vtkInteractorStyleRubberBand3D> RubberBandStyle;
  vtkSmartPointer<vtkInteractorObserver> CustomStyle;
  bool UseLightKitFlag;
  bool FirstRender;
  int InteractionMode;
};

VTK_ABI_NAMESPACE_END
#endif
