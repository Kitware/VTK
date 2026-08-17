// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkScivisDataRepresentation
 * @brief   A representation of data, and the contract a view reads it through.
 *
 * vtkScivisDataRepresentation is a vtkScivisRepresentation with data behind it:
 * an input to draw, a selection that can be made in it, and colors that come
 * from an array.  Everything a view does across representations -- sharing a
 * color map between two of them, labelling a scalar bar with a merged range,
 * framing the camera on the whole scene -- is built out of the questions
 * declared here.
 *
 * They are all required.  A representation that draws data can answer every one
 * of them, and one that cannot is an annotation, which derives from
 * vtkScivisRepresentation instead and is never asked.
 *
 * @sa vtkScivisRepresentation vtkScivisView vtkSurfaceRepresentation
 * vtkVolumeRepresentation
 */

#ifndef vtkScivisDataRepresentation_h
#define vtkScivisDataRepresentation_h

#include "vtkNew.h" // For ivars
#include "vtkScivisRepresentation.h"
#include "vtkSmartPointer.h"      // For ivars
#include "vtkViewsScivisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkAlgorithmOutput;
class vtkPlane;
class vtkPlaneCollection;
class vtkScalarsToColors;
class vtkScivisView;
class vtkSelection;

class VTKVIEWSSCIVIS_EXPORT vtkScivisDataRepresentation : public vtkScivisRepresentation
{
public:
  vtkTypeMacro(vtkScivisDataRepresentation, vtkScivisRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The extent of what this representation draws, in world coordinates, as
   * (xmin, xmax, ymin, ymax, zmin, zmax).
   *
   * Returns false and leaves @a bounds untouched when there is no extent to
   * report -- a representation drawn in screen space, or one whose input has
   * not arrived yet.
   */
  virtual bool GetBounds(double bounds[6]) = 0;

  ///@{
  /**
   * The array being mapped to colors, and one of the
   * vtkDataObject::FIELD_ASSOCIATION_* values for the attributes it comes from.
   *
   * Report what is actually being drawn rather than what was asked for.  A
   * representation that was told to color by an array reports that array; one
   * that was told nothing reports whatever it falls back to, which is usually
   * the active scalars.  A scalar bar has to be labelled with what is on
   * screen, so reporting the fallback is what makes the common case -- data
   * straight out of a reader, no array ever selected -- come out with a bar.
   *
   * The association means nothing while the name is null, which is what a
   * representation reports when it has nothing to color by yet.
   */
  virtual const char* GetRenderedArrayName() = 0;
  virtual int GetRenderedFieldAssociation() = 0;
  ///@}

  /**
   * The range of @a arrayName over the data this representation draws, so that
   * several representations of one array can be given a range that covers all
   * of them.  @a fieldAssoc is one of the vtkDataObject::FIELD_ASSOCIATION_*
   * values, and @a component selects a component of a multi-component array,
   * -1 asking for the magnitude.
   *
   * Report over the input rather than over whatever geometry is extracted from
   * it, so that two representations drawing the same array agree on its range
   * even when one of them only draws a surface of it.
   *
   * Returns false and leaves @a range untouched when the array is not there.
   */
  virtual bool GetDataRange(
    const char* arrayName, int fieldAssoc, double range[2], int component = -1) = 0;

  ///@{
  /**
   * The color map this representation draws through.
   *
   * SetColorMap() is an offer, not an instruction.  A view hands the same map
   * to every representation drawing the same array, so that they come out the
   * same color; a representation that owns transfer functions of its own is
   * free to ignore it.  Whatever GetColorMap() reports afterwards is what the
   * view labels its scalar bar with, and a view sets a range only on a map that
   * was taken up -- so ignoring the offer leaves both your colors and your
   * range alone.
   *
   */
  virtual void SetColorMap(vtkScalarsToColors* map) = 0;
  virtual vtkScalarsToColors* GetColorMap() = 0;
  ///@}

  ///@{
  /**
   * Planes that cut away part of what this representation draws.  Anything on
   * the negative side of a plane is not drawn; up to six planes apply at once.
   *
   * This is the one piece of the contract implemented here rather than left to
   * subclasses, because every mapper carries the same clipping and there is
   * nothing to be gained by each representation forwarding six methods of its
   * own.  A subclass hands GetClippingPlanes() to its mapper once, and the
   * mapper follows the collection from then on.  A representation drawn in
   * screen space simply never consults them.
   *
   * SetClippingPlanes() copies the planes into the collection this
   * representation owns rather than adopting the collection itself, so the
   * collection a subclass wired up stays the one in use.  Mutating the
   * collection returned by GetClippingPlanes() works and takes effect on the
   * next render.
   */
  void AddClippingPlane(vtkPlane* plane);
  void RemoveClippingPlane(vtkPlane* plane);
  void RemoveAllClippingPlanes();
  void SetClippingPlanes(vtkPlaneCollection* planes);
  vtkPlaneCollection* GetClippingPlanes();
  int GetNumberOfClippingPlanes();
  ///@}

  /**
   * A shallow copy of an input, kept up to date, that the filters inside a
   * representation can be connected to without holding its own pipeline open.
   */
  vtkAlgorithmOutput* GetInternalOutputPort() { return this->GetInternalOutputPort(0); }
  vtkAlgorithmOutput* GetInternalOutputPort(int port)
  {
    return this->GetInternalOutputPort(port, 0);
  }
  virtual vtkAlgorithmOutput* GetInternalOutputPort(int port, int conn);

  ///@{
  /**
   * Whether a view may select in this representation at all.  Default is on.
   */
  vtkSetMacro(Selectable, bool);
  vtkGetMacro(Selectable, bool);
  vtkBooleanMacro(Selectable, bool);
  ///@}

  /**
   * Hand this representation a selection made in @a view.  It is converted into
   * this representation's own terms and becomes the current one; @a extend adds
   * to what is already selected rather than replacing it.  Subclasses override
   * ConvertSelection() rather than this.
   */
  void Select(vtkScivisView* view, vtkSelection* selection)
  {
    this->Select(view, selection, false);
  }
  void Select(vtkScivisView* view, vtkSelection* selection, bool extend);

  /**
   * What this representation currently has selected, or null.
   */
  vtkSelection* GetCurrentSelection();

protected:
  vtkScivisDataRepresentation();
  ~vtkScivisDataRepresentation() override;

  /**
   * Turn a selection made in @a view into one expressed over this
   * representation's own input.  The default returns it unchanged; return null
   * to reject it.
   */
  virtual vtkSelection* ConvertSelection(vtkScivisView* view, vtkSelection* selection);

  /**
   * Make @a selection current and fire SelectionChangedEvent.
   */
  virtual void UpdateSelection(vtkSelection* selection, bool extend);

  vtkNew<vtkPlaneCollection> ClippingPlanes;
  vtkSmartPointer<vtkSelection> CurrentSelection;
  bool Selectable;

private:
  friend class vtkScivisView;

  vtkScivisDataRepresentation(const vtkScivisDataRepresentation&) = delete;
  void operator=(const vtkScivisDataRepresentation&) = delete;

  class Internals;
  Internals* Implementation;
};

VTK_ABI_NAMESPACE_END
#endif
