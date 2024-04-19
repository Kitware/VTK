// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkCirclePackFrontChainLayoutStrategy
 * @brief   layout a vtkTree into packed circles
 * using the front chain algorithm.
 *
 *
 * vtkCirclePackFrontChainLayoutStrategy assigns circles to each node of the input vtkTree
 * using the front chain algorithm.  The algorithm packs circles by searching a "front
 * chain" of circles around the perimeter of the circles that have already been packed for
 * the current level in the tree hierarchy.  Searching the front chain is in general faster
 * than searching all of the circles that have been packed at the current level.
 *
 * WARNING:  The algorithm tends to break down and produce packings with overlapping
 * circles when there is a large difference in the radii of the circles at a given
 * level of the tree hierarchy.  Roughly on the order a 1000:1 ratio of circle radii.
 *
 * Please see the following reference for more details on the algorithm.
 *
 * Title: "Visualization of large hierarchical data by circle packing"
 * Authors:  Weixin Wang, Hui Wang, Guozhong Dai, Hongan Wang
 * Conference: Proceedings of the SIGCHI conference on Human Factors in computing systems
 * Year: 2006
 *
 */

#ifndef vtkCirclePackFrontChainLayoutStrategy_h
#define vtkCirclePackFrontChainLayoutStrategy_h

#include "vtkCirclePackLayoutStrategy.h"
#include "vtkInfovisLayoutModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkCirclePackFrontChainLayoutStrategyImplementation;

class VTKINFOVISLAYOUT_EXPORT vtkCirclePackFrontChainLayoutStrategy
  : public vtkCirclePackLayoutStrategy
{
public:
  static vtkCirclePackFrontChainLayoutStrategy* New();

  vtkTypeMacro(vtkCirclePackFrontChainLayoutStrategy, vtkCirclePackLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform the layout of the input tree, and store the circle
   * bounds of each vertex as a tuple in a data array.
   * (Xcenter, Ycenter, Radius).
   */
  void Layout(vtkTree* inputTree, vtkDataArray* areaArray, vtkDataArray* sizeArray) override;

  ///@{
  /**
   * Width and Height define the size of the output window that the
   * circle packing is placed inside.  Defaults to Width 1, Height 1
   */
  vtkGetMacro(Width, int);
  vtkSetMacro(Width, int);
  vtkGetMacro(Height, int);
  vtkSetMacro(Height, int);
  ///@}

protected:
  vtkCirclePackFrontChainLayoutStrategy();
  ~vtkCirclePackFrontChainLayoutStrategy() override;

  char* CirclesFieldName;
  int Width;
  int Height;

private:
  vtkCirclePackFrontChainLayoutStrategyImplementation* pimpl; // Private implementation

  vtkCirclePackFrontChainLayoutStrategy(const vtkCirclePackFrontChainLayoutStrategy&) = delete;
  void operator=(const vtkCirclePackFrontChainLayoutStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
