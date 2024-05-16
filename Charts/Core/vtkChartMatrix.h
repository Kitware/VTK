// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
#include "vtkRect.h"             // for ivars
#include "vtkVector.h"           // For ivars
#include "vtkWrappingHints.h"    // For VTK_MARSHALAUTO

#include <map>     // For specific gutter
#include <utility> // For specific gutter

VTK_ABI_NAMESPACE_BEGIN
class vtkChart;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkChartMatrix : public vtkAbstractContextItem
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
   * Set the gutter that should be left between the charts in the matrix.
   */
  virtual void SetPadding(const float& padding);
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
   * Set the span of an element in the matrix. This defaults to 1x1, and cannot
   * exceed the remaining space in x or y.
   * \return false If the span is not possible.
   */
  virtual bool SetChartSpan(const vtkVector2i& position, const vtkVector2i& span);

  /**
   * Get the span of the specified element.
   */
  virtual vtkVector2i GetChartSpan(const vtkVector2i& position);

  /**
   * Get the position of an element in the matrix at the specified location.
   * The position should be specified in scene coordinates.
   */
  virtual vtkVector2i GetChartIndex(const vtkVector2f& position);

  /**
   * Get internal 1-D index corresponding to the 2-D element index.
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
   * The chart at index2 will be setup to mimic
   * axis range of chart at index1 for specified axis.
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
   * Unlink the two charts for specified axis i.e,
   * Chart at index2 will no longer mimic the axis range of chart at index1
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

  ///@{
  /**
   * Set the rectangular region that this chart matrix will occupy.
   * Must also set FillStrategy to StretchType::CUSTOM
   */
  virtual void SetRect(vtkRecti rect);
  vtkGetMacro(Rect, vtkRecti);
  ///@}

  /**
   * Set the element at position to a chart matrix,
   * note that the chart matrix must be large enough to
   * accommodate the element being set. Note that this class will take ownership
   * of the chart matrix object.
   * \return false if the element cannot be set.
   */
  virtual bool SetChartMatrix(const vtkVector2i& position, vtkChartMatrix* chartMatrix);

  /**
   * Get the specified chart matrix element. if the element does not exist, nullptr
   * will be returned. If the element has not yet been allocated it will be at this
   * point
   */
  virtual vtkChartMatrix* GetChartMatrix(const vtkVector2i& position);

  ///@{
  /**
   * These methods offer an API to iterate over the layout and obtain
   * the offset of each child element (chart or chart matrix) within the scene,
   * the index and the increment b/w each element.
   */
  virtual void InitLayoutTraversal(vtkVector2i& index, vtkVector2f& offset, vtkVector2f& increment);
  virtual void GoToNextElement(vtkVector2i& index, vtkVector2f& offset);
  virtual bool IsDoneWithTraversal();
  ///@}

  /**
   * Override this method if you want to customize layout instead of the default.
   * The returned rect will be in scene coordinates and suitable for a chart element
   * or chart matrix element.
   */
  virtual vtkRectf ComputeCurrentElementSceneRect(
    const vtkVector2i& index, const vtkVector2f& offset, const vtkVector2f& increment);

  enum class StretchType : unsigned int
  {
    SCENE = 0,
    CUSTOM
  };
  ///@{
  /**
   * This specifies whether the chart matrix will fill the entire scene
   * or instead draw itself in a user provided rectangular subset of the scene.
   */
  vtkSetEnumMacro(FillStrategy, StretchType);
  StretchType GetFillStrategy() { return this->FillStrategy; }
  ///@}

protected:
  vtkChartMatrix();
  ~vtkChartMatrix() override;

  // The number of charts in x and y.
  vtkVector2i Size;

  // The gutter between each chart.
  vtkVector2f Gutter;

  // The padding used inside each chart
  float Padding;
  std::map<vtkVector2i, vtkVector2f> SpecificResize;
  int Borders[4];
  bool LayoutIsDirty;

  // The rectangular region to occupy. (in scene coordinates.)
  vtkRecti Rect = { 0, 0, 100, 100 };
  StretchType FillStrategy = StretchType::SCENE;

  virtual void SynchronizeAxisRanges(vtkObject* caller, unsigned long eventId, void* calldata);

private:
  vtkChartMatrix(const vtkChartMatrix&) = delete;
  void operator=(const vtkChartMatrix&) = delete;

  class PIMPL;
  PIMPL* Private;
};

VTK_ABI_NAMESPACE_END
#endif // vtkChartMatrix_h
