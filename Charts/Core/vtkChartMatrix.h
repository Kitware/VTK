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

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkAbstractContextItem.h"
#include "vtkVector.h" // For ivars

#include <map> // For specific gutter
#include <utility> // For specific gutter

class vtkChart;

class VTKCHARTSCORE_EXPORT vtkChartMatrix : public vtkAbstractContextItem
{
public:
  vtkTypeMacro(vtkChartMatrix, vtkAbstractContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Creates a new object.
   */
  static vtkChartMatrix *New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   */
  virtual void Update();

  /**
   * Paint event for the chart matrix.
   */
  virtual bool Paint(vtkContext2D *painter);

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

  //@{
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
    for(int i=0;i<4;i++)
    {
      borders[i]=this->Borders[i];
    }
  }
  //@}

  //@{
  /**
   * Set the gutter that should be left between the charts in the matrix.
   */
  virtual void SetGutter(const vtkVector2f& gutter);
  void SetGutterX(float value);
  void SetGutterY(float value);
  //@}

  //@{
  /**
   * Set a specific resize that will move the bottom left point of a chart.
   */
  virtual void SetSpecificResize(const vtkVector2i& index, const vtkVector2f& resize);
  virtual void ClearSpecificResizes();
  //@}

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
   * Get the specified chart element, if the element does not exist NULL will be
   * returned. If the chart element has not yet been allocated it will be at
   * this point.
   */
  virtual vtkChart* GetChart(const vtkVector2i& position);

  /**
   * Set the span of a chart in the matrix. This defaults to 1x1, and cannot
   * exceed the remaining space in x or y.
   * \return false If the span is not possible.
   */
  virtual bool SetChartSpan(const vtkVector2i& position,
                            const vtkVector2i& span);

  /**
   * Get the span of the specified chart.
   */
  virtual vtkVector2i GetChartSpan(const vtkVector2i& position);

  /**
   * Get the position of the chart in the matrix at the specified location.
   * The position should be specified in scene coordinates.
   */
  virtual vtkVector2i GetChartIndex(const vtkVector2f& position);

protected:
  vtkChartMatrix();
  ~vtkChartMatrix();

  class PIMPL;
  PIMPL *Private;

  // The number of charts in x and y.
  vtkVector2i Size;

  // The gutter between each chart.
  vtkVector2f Gutter;
  std::map<vtkVector2i, vtkVector2f> SpecificResize;
  int Borders[4];
  bool LayoutIsDirty;

private:
  vtkChartMatrix(const vtkChartMatrix &) VTK_DELETE_FUNCTION;
  void operator=(const vtkChartMatrix &) VTK_DELETE_FUNCTION;
};

#endif //vtkChartMatrix_h
