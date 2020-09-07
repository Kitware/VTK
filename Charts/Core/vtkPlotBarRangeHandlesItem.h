/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotBarRangeHandlesItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkPlotBarRangeHandlesItem
 * @brief   show and control the range of a vtkAxis used with a vtkPlotBar
 *
 * This class is a vtkPlotRangeHandlesItem specialization working in
 * coordination with a vtkPlotBar. It ensures that handles are sticking to the
 * plot bars when being dragged.
 * Vertical and horizontal bars are both supported but the handles orientation
 * must match the vtkPlotBar orientation.
 *
 * @sa
 * vtkPlotRangeHandlesItem
 */

#ifndef vtkPlotBarRangeHandlesItem_h
#define vtkPlotBarRangeHandlesItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlotBar.h"          // User defined plotbar
#include "vtkPlotRangeHandlesItem.h"
#include "vtkSmartPointer.h" // Needed for vtkSmartPointer attribute

class VTKCHARTSCORE_EXPORT vtkPlotBarRangeHandlesItem : public vtkPlotRangeHandlesItem
{
public:
  vtkTypeMacro(vtkPlotBarRangeHandlesItem, vtkPlotRangeHandlesItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPlotBarRangeHandlesItem* New();

  /**
   * Recover the bounds of the item
   */
  void GetBounds(double bounds[4]) override;

  //@{
  /**
   * Get/Set the plot bar object to work with.
   */
  vtkSetSmartPointerMacro(PlotBar, vtkPlotBar);
  vtkGetSmartPointerMacro(PlotBar, vtkPlotBar);
  //@}

protected:
  vtkPlotBarRangeHandlesItem() = default;
  ~vtkPlotBarRangeHandlesItem() override = default;

  /**
   * Internal method to set the ActiveHandlePosition
   * and compute the ActiveHandleRangeValue accordingly
   */
  void SetActiveHandlePosition(double position) override;

private:
  vtkPlotBarRangeHandlesItem(const vtkPlotBarRangeHandlesItem&) = delete;
  void operator=(const vtkPlotBarRangeHandlesItem&) = delete;

  vtkSmartPointer<vtkPlotBar> PlotBar;
};

#endif // vtkPlotBarRangeHandlesItem_h
