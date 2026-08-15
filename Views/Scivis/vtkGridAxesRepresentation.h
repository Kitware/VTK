// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkGridAxesRepresentation
 * @brief   Labelled axes drawn around the scene.
 *
 * vtkGridAxesRepresentation draws a graduated box around what a view is
 * showing, so that the size and position of the data can be read off it.  It is
 * added to a view like anything else:
 *
 * @code
 * vtkNew<vtkGridAxesRepresentation> axes;
 * axes->GetGridAxesActor()->SetXTitle("x (m)");
 * view->AddRepresentation(axes);
 * @endcode
 *
 * @par It follows the scene:
 * There is nothing to connect and no bounds to set.  The axes listen for the
 * view's BoundsChangedEvent and resize themselves whenever what is being drawn
 * changes -- data arriving, a representation being hidden, a reader stepping to
 * the next timestep.
 *
 * @par Why it does not measure itself:
 * The bounds come from the view, which takes them from the representations that
 * have data.  Asking the renderer instead would include these axes, which are
 * drawn around the data and so are always larger than it, and each render would
 * grow the axes to fit the axes.  Being a representation with no data of its
 * own is what keeps it out of the measurement it depends on.
 *
 * @par Readable by default:
 * The labels and titles start at 16 and 20 points rather than the 12 that
 * vtkTextProperty defaults to and vtkGridAxesActor3D leaves alone, which is
 * small for numbers meant to be read off a scene.  Both are text properties on
 * the actor, so an application that wants otherwise says so through it.
 *
 * @par What is here and what is not:
 * Whether the axes are drawn, and how much room to leave around the data.
 * Everything about how they look -- which faces are drawn, titles, label
 * formats, fonts, grid lines -- belongs to vtkGridAxesActor3D, which
 * GetGridAxesActor() hands out rather than mirroring it one method at a time.
 *
 * @sa vtkScivisRepresentation vtkScivisView vtkGridAxesActor3D
 */

#ifndef vtkGridAxesRepresentation_h
#define vtkGridAxesRepresentation_h

#include "vtkNew.h" // For ivar
#include "vtkScivisRepresentation.h"
#include "vtkViewsScivisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkGridAxesActor3D;

class VTKVIEWSSCIVIS_EXPORT vtkGridAxesRepresentation : public vtkScivisRepresentation
{
public:
  static vtkGridAxesRepresentation* New();
  vtkTypeMacro(vtkGridAxesRepresentation, vtkScivisRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Whether the axes are drawn.  Default is on.
   */
  void SetVisibility(bool val) override;
  bool GetVisibility() override;
  ///@}

  ///@{
  /**
   * How much room to leave between the data and the axes, as a fraction of the
   * size of the data.  Default is 0, which puts the axes right against it.
   */
  void SetPadding(double padding);
  double GetPadding();
  ///@}

  /**
   * The actor the axes are drawn by: which faces are drawn, the titles, the
   * label formats, the fonts and the grid lines.
   */
  vtkGridAxesActor3D* GetGridAxesActor();

protected:
  vtkGridAxesRepresentation();
  ~vtkGridAxesRepresentation() override;

  bool AddToView(vtkScivisView* view) override;
  bool RemoveFromView(vtkScivisView* view) override;

private:
  vtkGridAxesRepresentation(const vtkGridAxesRepresentation&) = delete;
  void operator=(const vtkGridAxesRepresentation&) = delete;

  /**
   * Remember the size of the scene and fit the axes around it.  Called when the
   * view says its scene has changed size.
   */
  void OnBoundsChanged(vtkObject* caller, unsigned long event, void* bounds);

  /**
   * Fit the axes around the scene as last reported, leaving Padding around it.
   * The scene is remembered rather than asked for, because a representation has
   * no pointer back to its view -- and because padding can change when the
   * scene has not, which would otherwise not show until it next did.
   */
  void Fit();

  vtkNew<vtkGridAxesActor3D> Actor;
  double Padding;
  double SceneBounds[6];
  bool HasSceneBounds;
  // The observer on the view, so it can be taken off again when the axes are
  // removed from it.
  unsigned long ObserverTag;
};

VTK_ABI_NAMESPACE_END
#endif
