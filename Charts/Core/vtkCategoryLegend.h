// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkCategoryLegend
 * @brief   Legend item to display categorical data.
 *
 * vtkCategoryLegend will display a label and color patch for each value
 * in a categorical data set.  To use this class, you must first populate
 * a vtkScalarsToColors by using the SetAnnotation() method.  The other
 * input to this class is a vtkVariantArray.  This should contain the
 * annotated values from the vtkScalarsToColors that you wish to include
 * within the legend.
 */

#ifndef vtkCategoryLegend_h
#define vtkCategoryLegend_h

#include "vtkChartLegend.h"
#include "vtkChartsCoreModule.h" // For export macro
#include "vtkNew.h"              // For vtkNew ivars
#include "vtkStdString.h"        // For vtkStdString ivars
#include "vtkVector.h"           // For vtkRectf
#include "vtkWrappingHints.h"    // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkScalarsToColors;
class vtkTextProperty;
class vtkVariantArray;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkCategoryLegend : public vtkChartLegend
{
public:
  vtkTypeMacro(vtkCategoryLegend, vtkChartLegend);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkCategoryLegend* New();

  /**
   * Enum of legend orientation types
   */
  enum
  {
    VERTICAL = 0,
    HORIZONTAL
  };

  /**
   * Paint the legend into a rectangle defined by the bounds.
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Compute and return the lower left corner of this legend, along
   * with its width and height.
   */
  vtkRectf GetBoundingRect(vtkContext2D* painter) override;

  ///@{
  /**
   * Get/Set the vtkScalarsToColors used to draw this legend.
   * Since this legend represents categorical data, this
   * vtkScalarsToColors must have been populated using SetAnnotation().
   */
  virtual void SetScalarsToColors(vtkScalarsToColors* stc);
  virtual vtkScalarsToColors* GetScalarsToColors();
  ///@}

  ///@{
  /**
   * Get/Set the array of values that will be represented by this legend.
   * This array must contain distinct annotated values from the ScalarsToColors.
   * Each value in this array will be drawn as a separate entry within this
   * legend.
   */
  vtkGetObjectMacro(Values, vtkVariantArray);
  virtual void SetValues(vtkVariantArray*);
  ///@}

  ///@{
  /**
   * Get/set the title text of the legend.
   */
  virtual void SetTitle(const vtkStdString& title);
  virtual vtkStdString GetTitle();
  ///@}

  ///@{
  /**
   * Get/set the label to use for outlier data.
   */
  vtkGetMacro(OutlierLabel, vtkStdString);
  vtkSetMacro(OutlierLabel, vtkStdString);
  ///@}

protected:
  vtkCategoryLegend();
  ~vtkCategoryLegend() override;

  bool HasOutliers;
  float TitleWidthOffset;
  vtkScalarsToColors* ScalarsToColors;
  vtkStdString OutlierLabel;
  vtkStdString Title;
  vtkNew<vtkTextProperty> TitleProperties;
  vtkVariantArray* Values;

private:
  vtkCategoryLegend(const vtkCategoryLegend&) = delete;
  void operator=(const vtkCategoryLegend&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
