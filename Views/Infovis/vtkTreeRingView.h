// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkTreeRingView
 * @brief   Displays a tree in concentric rings.
 *
 *
 * Accepts a graph and a hierarchy - currently
 * a tree - and provides a hierarchy-aware display.  Currently, this means
 * displaying the hierarchy using a tree ring layout, then rendering the graph
 * vertices as leaves of the tree with curved graph edges between leaves.
 *
 * .SEE ALSO
 * vtkGraphLayoutView
 *
 * @par Thanks:
 * Thanks to Jason Shepherd for implementing this class
 */

#ifndef vtkTreeRingView_h
#define vtkTreeRingView_h

#include "vtkTreeAreaView.h"
#include "vtkViewsInfovisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKVIEWSINFOVIS_EXPORT vtkTreeRingView : public vtkTreeAreaView
{
public:
  static vtkTreeRingView* New();
  vtkTypeMacro(vtkTreeRingView, vtkTreeAreaView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the root angles for laying out the hierarchy.
   */
  void SetRootAngles(double start, double end);

  ///@{
  /**
   * Sets whether the root is at the center or around the outside.
   */
  virtual void SetRootAtCenter(bool center);
  virtual bool GetRootAtCenter();
  vtkBooleanMacro(RootAtCenter, bool);
  ///@}

  ///@{
  /**
   * Set the thickness of each layer.
   */
  virtual void SetLayerThickness(double thickness);
  virtual double GetLayerThickness();
  ///@}

  ///@{
  /**
   * Set the interior radius of the tree
   * (i.e. the size of the "hole" in the center).
   */
  virtual void SetInteriorRadius(double rad);
  virtual double GetInteriorRadius();
  ///@}

  ///@{
  /**
   * Set the log spacing factor for the invisible interior tree
   * used for routing edges of the overlaid graph.
   */
  virtual void SetInteriorLogSpacingValue(double value);
  virtual double GetInteriorLogSpacingValue();
  ///@}

protected:
  vtkTreeRingView();
  ~vtkTreeRingView() override;

private:
  vtkTreeRingView(const vtkTreeRingView&) = delete;
  void operator=(const vtkTreeRingView&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
