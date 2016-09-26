/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkRenderView
 * @brief   A view containing a renderer.
 *
 *
 * vtkRenderView is a view which contains a vtkRenderer.  You may add vtkActors
 * directly to the renderer, or add certain vtkDataRepresentation subclasses
 * to the renderer.  The render view supports drag selection with the mouse to
 * select cells.
 *
 * This class is also the parent class for any more specialized view which uses
 * a renderer.
*/

#ifndef vtkRenderView_h
#define vtkRenderView_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkRenderViewBase.h"
#include "vtkSmartPointer.h" // For SP ivars

class vtkAbstractTransform;
class vtkActor2D;
class vtkAlgorithmOutput;
class vtkArrayCalculator;
class vtkBalloonRepresentation;
class vtkDynamic2DLabelMapper;
class vtkHardwareSelector;
class vtkHoverWidget;
class vtkInteractorObserver;
class vtkLabelPlacementMapper;
class vtkPolyDataMapper2D;
class vtkSelection;
class vtkTextProperty;
class vtkTexture;
class vtkTexturedActor2D;
class vtkTransformCoordinateSystems;

class VTKVIEWSINFOVIS_EXPORT vtkRenderView : public vtkRenderViewBase
{
public:
  static vtkRenderView* New();
  vtkTypeMacro(vtkRenderView, vtkRenderViewBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * The render window interactor. Note that this requires special
   * handling in order to do correctly - see the notes in the detailed
   * description of vtkRenderViewBase.
   */
  virtual void SetInteractor(vtkRenderWindowInteractor *interactor);

  /**
   * The interactor style associated with the render view.
   */
  virtual void SetInteractorStyle(vtkInteractorObserver* style);

  /**
   * Get the interactor style associated with the render view.
   */
  virtual vtkInteractorObserver* GetInteractorStyle();

  /**
   * Set the render window for this view. Note that this requires special
   * handling in order to do correctly - see the notes in the detailed
   * description of vtkRenderViewBase.
   */
  virtual void SetRenderWindow(vtkRenderWindow *win);

  enum
  {
    INTERACTION_MODE_2D,
    INTERACTION_MODE_3D,
    INTERACTION_MODE_UNKNOWN
  };

  void SetInteractionMode(int mode);
  vtkGetMacro(InteractionMode, int);

  /**
   * Set the interaction mode for the view. Choices are:
   * vtkRenderView::INTERACTION_MODE_2D - 2D interactor
   * vtkRenderView::INTERACTION_MODE_3D - 3D interactor
   */
  virtual void SetInteractionModeTo2D()
    { this->SetInteractionMode(INTERACTION_MODE_2D); }
  virtual void SetInteractionModeTo3D()
    { this->SetInteractionMode(INTERACTION_MODE_3D); }

  /**
   * Updates the representations, then calls Render() on the render window
   * associated with this view.
   */
  virtual void Render();

  /**
   * Applies a view theme to this view.
   */
  virtual void ApplyViewTheme(vtkViewTheme* theme);

  //@{
  /**
   * Set the view's transform. All vtkRenderedRepresentations
   * added to this view should use this transform.
   */
  virtual void SetTransform(vtkAbstractTransform* transform);
  vtkGetObjectMacro(Transform, vtkAbstractTransform);
  //@}

  //@{
  /**
   * Whether the view should display hover text.
   */
  virtual void SetDisplayHoverText(bool b);
  vtkGetMacro(DisplayHoverText, bool);
  vtkBooleanMacro(DisplayHoverText, bool);
  //@}

  enum {
    SURFACE = 0,
    FRUSTUM = 1
  };

  //@{
  /**
   * Sets the selection mode for the render view.
   * SURFACE selection uses vtkHardwareSelector to perform a selection
   * of visible cells.
   * FRUSTUM selection just creates a view frustum selection, which will
   * select everything in the frustum.
   */
  vtkSetClampMacro(SelectionMode, int, 0, 1);
  vtkGetMacro(SelectionMode, int);
  void SetSelectionModeToSurface() { this->SetSelectionMode(SURFACE); }
  void SetSelectionModeToFrustum() { this->SetSelectionMode(FRUSTUM); }
  //@}

  /**
   * Add labels from an input connection with an associated text
   * property. The output must be a vtkLabelHierarchy (normally the
   * output of vtkPointSetToLabelHierarchy).
   */
  virtual void AddLabels(vtkAlgorithmOutput* conn);

  /**
   * Remove labels from an input connection.
   */
  virtual void RemoveLabels(vtkAlgorithmOutput* conn);

  //@{
  /**
   * Set the icon sheet to use for rendering icons.
   */
  virtual void SetIconTexture(vtkTexture* texture);
  vtkGetObjectMacro(IconTexture, vtkTexture);
  //@}

  //@{
  /**
   * Set the size of each icon in the icon texture.
   */
  vtkSetVector2Macro(IconSize, int);
  vtkGetVector2Macro(IconSize, int);
  //@}

  //@{
  /**
   * Set the display size of the icon (which may be different from the icon
   * size). By default, if this value is not set, the the IconSize is used.
   */
  vtkSetVector2Macro(DisplaySize, int);
  int* GetDisplaySize();
  void GetDisplaySize(int &dsx, int &dsy);
  //@}

  enum
  {
    NO_OVERLAP,
    ALL
  };

  //@{
  /**
   * Label placement mode.
   * NO_OVERLAP uses vtkLabelPlacementMapper, which has a faster startup time and
   * works with 2D or 3D labels.
   * ALL displays all labels (Warning: This may cause incredibly slow render
   * times on datasets with more than a few hundred labels).
   */
  virtual void SetLabelPlacementMode(int mode);
  virtual int GetLabelPlacementMode();
  virtual void SetLabelPlacementModeToNoOverlap()
    { this->SetLabelPlacementMode(NO_OVERLAP); }
  virtual void SetLabelPlacementModeToAll()
    { this->SetLabelPlacementMode(ALL); }
  //@}

  enum
  {
    FREETYPE,
    QT
  };

  //@{
  /**
   * Label render mode.
   * FREETYPE uses the freetype label rendering.
   * QT uses more advanced Qt-based label rendering.
   */
  virtual void SetLabelRenderMode(int mode);
  virtual int GetLabelRenderMode();
  virtual void SetLabelRenderModeToFreetype()
    { this->SetLabelRenderMode(FREETYPE); }
  virtual void SetLabelRenderModeToQt()
    { this->SetLabelRenderMode(QT); }
  //@}

  //@{
  /**
   * Whether to render on every mouse move.
   */
  void SetRenderOnMouseMove(bool b);
  vtkGetMacro(RenderOnMouseMove, bool);
  vtkBooleanMacro(RenderOnMouseMove, bool);
  //@}

protected:
  vtkRenderView();
  ~vtkRenderView();

  /**
   * Called to process events.
   * Captures StartEvent events from the renderer and calls Update().
   * This may be overridden by subclasses to process additional events.
   */
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId,
    void* callData);

  /**
   * Generates the selection based on the view event and the selection mode.
   */
  virtual void GenerateSelection(
    void* callData, vtkSelection* selection);

  /**
   * Called by the view when the renderer is about to render.
   */
  virtual void PrepareForRendering();

  /**
   * Called in PrepareForRendering to update the hover text.
   */
  virtual void UpdateHoverText();

  /**
   * Enable or disable hovering based on DisplayHoverText ivar
   * and interaction state.
   */
  virtual void UpdateHoverWidgetState();

  /**
   * Update the pick render for queries for drag selections
   * or hover ballooons.
   */
  void UpdatePickRender();

  int SelectionMode;
  int LabelRenderMode;
  bool DisplayHoverText;
  bool Interacting;
  bool InHoverTextRender;
  bool InPickRender;
  bool PickRenderNeedsUpdate;

  vtkAbstractTransform* Transform;
  vtkTexture* IconTexture;
  int IconSize[2];
  int DisplaySize[2];

  int InteractionMode;
  bool RenderOnMouseMove;

  vtkSmartPointer<vtkRenderer>                 LabelRenderer;
  vtkSmartPointer<vtkBalloonRepresentation>    Balloon;
  vtkSmartPointer<vtkLabelPlacementMapper>     LabelPlacementMapper;
  vtkSmartPointer<vtkTexturedActor2D>          LabelActor;
  vtkSmartPointer<vtkHoverWidget>              HoverWidget;
  vtkSmartPointer<vtkHardwareSelector>         Selector;

private:
  vtkRenderView(const vtkRenderView&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRenderView&) VTK_DELETE_FUNCTION;
};

#endif
