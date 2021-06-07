/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartMatrix.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkChartMatrix
 * @brief   container for a matrix of charts.
 *
 *
 * This class contains a matrix of charts. These charts will be of type
 * vtkChartXY by default, but this can be overridden. The class will manage
 * their layout and object lifetime.
 */

#ifndef vtkChartMatrix_h
#define vtkChartMatrix_h

#include "vtkAbstractContextItem.h"
#include "vtkChartsCoreModule.h" // For export macro
#include "vtkVector.h"           // For ivars

#include <map>     // For specific gutter
#include <utility> // For specific gutter

class vtkChart;

class VTKCHARTSCORE_EXPORT vtkChartMatrix : public vtkAbstractContextItem
{
public:
  vtkTypeMacro(vtkChartMatrix, vtkAbstractContextItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a new object.
   */
  static vtkChartMatrix* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   */
  void Update() override;

  /**
   * Paint event for the chart matrix.
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Set the width and height of the chart matrix. This will cause an immediate
   * resize of the chart matrix, the default size is 0x0 (no charts). No chart
   * objects are created until Allocate is called.
   */
  virtual void SetSize(const vtkVector2i& size);

  /**
   * Get the width and height of the chart matrix.
   */
  virtual vtkVector2i GetSize() const { return this->Size; }

  ///@{
  /**
   * Set/get the borders of the chart matrix (space in pixels around each chart).
   */
  virtual void SetBorders(int left, int bottom, int right, int top);
  void SetBorderLeft(int value);
  void SetBorderBottom(int value);
  void SetBorderRight(int value);
  void SetBorderTop(int value);
  virtual void GetBorders(int borders[4])
  {
    for (int i = 0; i < 4; i++)
    {
      borders[i] = this->Borders[i];
    }
  }
  ///@}

  ///@{
  /**
   * Set the gutter that should be left between the charts in the matrix.
   */
  virtual void SetGutter(const vtkVector2f& gutter);
  void SetGutterX(float value);
  void SetGutterY(float value);
  ///@}

  ///@{
  /**
   * Set a specific resize that will move the bottom left point of a chart.
   */
  virtual void SetSpecificResize(const vtkVector2i& index, const vtkVector2f& resize);
  virtual void ClearSpecificResizes();
  ///@}

  /**
   * Get the gutter that should be left between the charts in the matrix.
   */
  virtual vtkVector2f GetGutter() const { return this->Gutter; }

  /**
   * Allocate the charts, this will cause any null chart to be allocated.
   */
  virtual void Allocate();

  /**
   * Set the chart element, note that the chart matrix must be large enough to
   * accommodate the element being set. Note that this class will take ownership
   * of the chart object.
   * \return false if the element cannot be set.
   */
  virtual bool SetChart(const vtkVector2i& position, vtkChart* chart);

  /**
   * Get the specified chart element, if the element does not exist nullptr will be
   * returned. If the chart element has not yet been allocated it will be at
   * this point.
   */
  virtual vtkChart* GetChart(const vtkVector2i& position);

  /**
   * Set the span of a chart in the matrix. This defaults to 1x1, and cannot
   * exceed the remaining space in x or y.
   * \return false If the span is not possible.
   */
  virtual bool SetChartSpan(const vtkVector2i& position, const vtkVector2i& span);

  /**
   * Get the span of the specified chart.
   */
  virtual vtkVector2i GetChartSpan(const vtkVector2i& position);

  /**
   * Get the position of the chart in the matrix at the specified location.
   * The position should be specified in scene coordinates.
   */
  virtual vtkVector2i GetChartIndex(const vtkVector2f& position);

  /**
   * Get internal 1-D index corresponding to the 2-D chart index.
   */
  virtual std::size_t GetFlatIndex(const vtkVector2i& index);

  /**
   * Total number of charts within this chart matrix.
   */
  virtual std::size_t GetNumberOfCharts();

  /**
   * Link all charts in the rectangle from leftBottom to rightTop.
   * Label only the outer most y-axis and x-axis.
   * This removes of gutter space between the linked charts.
   */
  virtual void LabelOuter(const vtkVector2i& leftBottomIdx, const vtkVector2i& rightTopIdx);

  ///@{
  /**
   * The chart at index1 will be setup to mimic
   * axis range of chart at index2 for specified axis.
   * Note: index is a two dimensional chart index. See vtkChartMatrix::GetChartIndex()
   *       flatIndex is a one dimensional chart index. See vtkChartMatrix::GetFlatIndex()
   */
  virtual void Link(const vtkVector2i& index1, const vtkVector2i& index2, int axis = 1);
  virtual void Link(const size_t& flatIndex1, const size_t& flatIndex2, int axis = 1);
  ///@}

  ///@{
  /**
   * Link a chart to all other charts in this chart matrix for specified axis
   */
  virtual void LinkAll(const vtkVector2i& index, int axis = 1);
  virtual void LinkAll(const size_t& flatIndex, int axis = 1);
  ///@}

  ///@{
  /**
   * Unlink the two charts for specified axis
   */
  virtual void Unlink(const vtkVector2i& index1, const vtkVector2i& index2, int axis = 1);
  virtual void Unlink(const size_t& flatIndex1, const size_t& flatIndex2, int axis = 1);
  ///@}

  ///@{
  /**
   * Unlink all charts from given chart for a specified axis.
   */
  virtual void UnlinkAll(const vtkVector2i& index, int axis = 1);
  virtual void UnlinkAll(const size_t& flatIndex, int axis = 1);
  ///@}

  ///@{
  /**
   * Unlink every chart from all other charts for a specified axis.
   * This effectively removes any linkage in the chart matrix.
   * If vtkChartMatrix::LabelOuter() was used, call ResetLinkedLayout,
   * sot that the gutters that were removed will
   * be put back in place.
   */
  virtual void ResetLinks(int axis = 1);
  virtual void ResetLinkedLayout();
  ///@}

protected:
  vtkChartMatrix();
  ~vtkChartMatrix() override;

  // The number of charts in x and y.
  vtkVector2i Size;

  // The gutter between each chart.
  vtkVector2f Gutter;
  std::map<vtkVector2i, vtkVector2f> SpecificResize;
  int Borders[4];
  bool LayoutIsDirty;

  virtual void SynchronizeAxisRanges(vtkObject* caller, unsigned long eventId, void* calldata);

private:
  vtkChartMatrix(const vtkChartMatrix&) = delete;
  void operator=(const vtkChartMatrix&) = delete;

  class PIMPL;
  PIMPL* Private;
};

#endif // vtkChartMatrix_h
