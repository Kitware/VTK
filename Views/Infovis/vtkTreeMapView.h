// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkTreeMapView
 * @brief   Displays a tree as a tree map.
 *
 *
 * vtkTreeMapView shows a vtkTree in a tree map, where each vertex in the
 * tree is represented by a box.  Child boxes are contained within the
 * parent box, and may be colored and sized by various parameters.
 */

#ifndef vtkTreeMapView_h
#define vtkTreeMapView_h

#include "vtkTreeAreaView.h"
#include "vtkViewsInfovisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkBoxLayoutStrategy;
class vtkSliceAndDiceLayoutStrategy;
class vtkSquarifyLayoutStrategy;

class VTKVIEWSINFOVIS_EXPORT vtkTreeMapView : public vtkTreeAreaView
{
public:
  static vtkTreeMapView* New();
  vtkTypeMacro(vtkTreeMapView, vtkTreeAreaView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Sets the treemap layout strategy
   */
  void SetLayoutStrategy(vtkAreaLayoutStrategy* s) override;
  virtual void SetLayoutStrategy(const char* name);
  virtual void SetLayoutStrategyToBox();
  virtual void SetLayoutStrategyToSliceAndDice();
  virtual void SetLayoutStrategyToSquarify();
  ///@}

  ///@{
  /**
   * The sizes of the fonts used for labeling.
   */
  virtual void SetFontSizeRange(int maxSize, int minSize, int delta = 4);
  virtual void GetFontSizeRange(int range[3]);
  ///@}

protected:
  vtkTreeMapView();
  ~vtkTreeMapView() override;

  vtkSmartPointer<vtkBoxLayoutStrategy> BoxLayout;
  vtkSmartPointer<vtkSliceAndDiceLayoutStrategy> SliceAndDiceLayout;
  vtkSmartPointer<vtkSquarifyLayoutStrategy> SquarifyLayout;

private:
  vtkTreeMapView(const vtkTreeMapView&) = delete;
  void operator=(const vtkTreeMapView&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
