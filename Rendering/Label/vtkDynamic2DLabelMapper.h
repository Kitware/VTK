// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkDynamic2DLabelMapper
 * @brief   draw text labels at 2D dataset points
 *
 * vtkDynamic2DLabelMapper is a mapper that renders text at dataset
 * points such that the labels do not overlap.
 * Various items can be labeled including point ids, scalars,
 * vectors, normals, texture coordinates, tensors, and field data components.
 * This mapper assumes that the points are located on the x-y plane
 * and that the camera remains perpendicular to that plane with a y-up
 * axis (this can be constrained using vtkImageInteractor).
 * On the first render, the mapper computes the visibility of all labels
 * at all scales, and queries this information on successive renders.
 * This causes the first render to be much slower. The visibility algorithm
 * is a greedy approach based on the point id, so the label for a point
 * will be drawn unless the label for a point with lower id overlaps it.
 *
 * @warning
 * Use this filter in combination with vtkSelectVisiblePoints if you want
 * to label only points that are visible. If you want to label cells rather
 * than points, use the filter vtkCellCenters to generate points at the
 * center of the cells. Also, you can use the class vtkGenerateIds to
 * generate ids as scalars or field data, which can then be labeled.
 *
 * @sa
 * vtkLabeledDataMapper
 *
 * @par Thanks:
 * This algorithm was developed in the paper
 * Ken Been and Chee Yap. Dynamic Map Labeling. IEEE Transactions on
 * Visualization and Computer Graphics, Vol. 12, No. 5, 2006. pp. 773-780.
 */

#ifndef vtkDynamic2DLabelMapper_h
#define vtkDynamic2DLabelMapper_h

#include "vtkLabeledDataMapper.h"
#include "vtkRenderingLabelModule.h" // For export macro
#include "vtkWrappingHints.h"        // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGLABEL_EXPORT VTK_MARSHALAUTO vtkDynamic2DLabelMapper : public vtkLabeledDataMapper
{
public:
  ///@{
  /**
   * Instantiate object with %%-#6.3g label format. By default, point ids
   * are labeled.
   */
  static vtkDynamic2DLabelMapper* New();
  vtkTypeMacro(vtkDynamic2DLabelMapper, vtkLabeledDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Set the points array name to use to give priority to labels.
   * Defaults to "priority".
   */
  void SetPriorityArrayName(const char* name);

  ///@{
  /**
   * Whether to reverse the priority order (i.e. low values have high priority).
   * Default is off.
   */
  vtkSetMacro(ReversePriority, bool);
  vtkGetMacro(ReversePriority, bool);
  vtkBooleanMacro(ReversePriority, bool);
  ///@}

  ///@{
  /**
   * Set the label height padding as a percentage. The percentage
   * is a percentage of your label height.
   * Default is 50%.
   */
  vtkSetMacro(LabelHeightPadding, float);
  vtkGetMacro(LabelHeightPadding, float);
  ///@}

  ///@{
  /**
   * Set the label width padding as a percentage. The percentage
   * is a percentage of your label ^height^ (yes, not a typo).
   * Default is 50%.
   */
  vtkSetMacro(LabelWidthPadding, float);
  vtkGetMacro(LabelWidthPadding, float);
  ///@}

  ///@{
  /**
   * Draw non-overlapping labels to the screen.
   */
  void RenderOpaqueGeometry(vtkViewport* viewport, vtkActor2D* actor) override;
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor) override;
  ///@}

protected:
  vtkDynamic2DLabelMapper();
  ~vtkDynamic2DLabelMapper() override;

  /**
   * Calculate the current zoom scale of the viewport.
   */
  double GetCurrentScale(vtkViewport* viewport);

  float* LabelWidth;
  float* LabelHeight;
  float* Cutoff;
  float ReferenceScale;
  float LabelHeightPadding;
  float LabelWidthPadding;

  bool ReversePriority;

private:
  vtkDynamic2DLabelMapper(const vtkDynamic2DLabelMapper&) = delete;
  void operator=(const vtkDynamic2DLabelMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
