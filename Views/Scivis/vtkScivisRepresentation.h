// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkScivisRepresentation
 * @brief   Something a view shows.
 *
 * A representation is a choice of how to show something.  You create one, point
 * it at data, configure how it should look, and add it to a view; from then on
 * the view draws it.  The same dataset shown as a surface and shown as a volume
 * is two representations -- the data is the same, the way of showing it is not.
 *
 * @par Why everything in a scene is one:
 * Data drawn as a surface, data drawn as a volume, and in time a text overlay,
 * a scale bar, a set of axes -- all of them are representations.  So all of
 * them are created, configured, added, shown and hidden in the same way, and a
 * view holds them in a single list and treats them alike.  Learn how one works
 * and you know how the rest do.
 *
 * @par Why it asks for so little:
 * Being showable ought to cost almost nothing, or the uniformity above is only
 * available to whatever was worth the price.  All a view needs of its contents
 * is that they can be shown and hidden, and that they can put themselves into a
 * view and take themselves out again, so that is all this class requires.  A
 * scale bar is showable in exactly that sense, and nothing is gained by making
 * it carry an input dataset, a color map and a selection round trip in order to
 * say so.  Those belong to vtkScivisDataRepresentation, which is what a
 * representation of actual data derives from.
 *
 * @par Writing one:
 * @code
 * class MyAnnotation : public vtkScivisRepresentation
 * {
 *   void SetVisibility(bool val) override;
 *   bool GetVisibility() override;
 *
 * protected:
 *   bool AddToView(vtkScivisView* view) override;     // add your props
 *   bool RemoveFromView(vtkScivisView* view) override;
 * };
 * @endcode
 *
 * @par A family of its own:
 * These are not vtkDataRepresentations and cannot be shown by the information
 * visualization views, which is deliberate: those views drive their own render
 * passes and labelling, carry annotation links and themes that mean nothing
 * here, and neither family can stand in for the other.  Sharing a base class
 * would have said otherwise.
 *
 * @sa vtkScivisDataRepresentation vtkScivisView
 */

#ifndef vtkScivisRepresentation_h
#define vtkScivisRepresentation_h

#include "vtkPassInputTypeAlgorithm.h"
#include "vtkViewsScivisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkScivisView;

class VTKVIEWSSCIVIS_EXPORT vtkScivisRepresentation : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkScivisRepresentation, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Whether this representation draws at all.
   *
   * A view leaves a hidden representation out of everything it does across the
   * scene, so hiding one also takes it out of the range a shared color map
   * spans and out of the bounds the camera frames.
   */
  virtual void SetVisibility(bool val) = 0;
  virtual bool GetVisibility() = 0;
  ///@}

protected:
  vtkScivisRepresentation();
  ~vtkScivisRepresentation() override;

  ///@{
  /**
   * Called when this representation is added to or removed from a view.
   * Returning false from AddToView() refuses the view, and the view drops the
   * representation again.
   */
  virtual bool AddToView(vtkScivisView* vtkNotUsed(view)) { return true; }
  virtual bool RemoveFromView(vtkScivisView* vtkNotUsed(view)) { return true; }
  ///@}

private:
  friend class vtkScivisView;

  vtkScivisRepresentation(const vtkScivisRepresentation&) = delete;
  void operator=(const vtkScivisRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
