/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxis.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/// \file vtkQtChartAxis.h
/// \date February 1, 2008

#ifndef _vtkQtChartAxis_h
#define _vtkQtChartAxis_h

#include "vtkQtChartExport.h"
#include <QObject>
#include <QGraphicsItem>

#include "vtkQtChartGraphicsItemTypes.h" // needed for enum

class vtkQtChartAxisInternal;
class vtkQtChartAxisModel;
class vtkQtChartAxisOptions;
class vtkQtChartContentsArea;
class vtkQtChartContentsSpace;
class QGraphicsLineItem;
class QVariant;


/// \class vtkQtChartAxis
/// \brief
///   The vtkQtChartAxis class is used to display a cartesian axis.
class VTKQTCHART_EXPORT vtkQtChartAxis : public QObject, public QGraphicsItem
{
  Q_OBJECT

public:
  enum AxisLocation
    {
    Left = 0, ///< The axis is on the left of the chart.
    Bottom,   ///< The axis is on the bottom of the chart.
    Right,    ///< The axis is on the right of the chart.
    Top       ///< The axis is on the top of the chart.
    };

  enum AxisDomain
    {
    UnsupportedDomain = -1,
    Number = 0, ///< Domain for int and double.
    Date,       ///< Domain for QDate and QDateTime.
    Time,       ///< Domain for QTime.
    String      ///< Domain for QString.
    };

  enum {Type = vtkQtChart_AxisType};

public:
  /// \brief
  ///   Creates a chart axis view.
  /// \param location Where on the chart the axis will be drawn.
  /// \param parent The parent item.
  vtkQtChartAxis(AxisLocation location, QGraphicsItem *parent=0);
  virtual ~vtkQtChartAxis();

  virtual int type() const {return vtkQtChartAxis::Type;}

  /// \name Setup Methods
  //@{
  /// \brief
  ///   Gets the axis location on the chart.
  /// \return
  ///   The axis location on the chart.
  AxisLocation getLocation() const {return this->Location;}

  /// \brief
  ///   Gets the chart axis model.
  /// \return
  ///   A pointer to the chart axis model.
  vtkQtChartAxisModel *getModel() const {return this->Model;}

  /// \brief
  ///   Sets the chart axis model.
  /// \param model The model to display.
  void setModel(vtkQtChartAxisModel *model);

  /// \brief
  ///   Sets the neighboring axes if any.
  ///
  /// The neighboring axes are used when laying out the axis. The
  /// axis pixel positions are adjusted to accound for the space
  /// requirements of its neighbors.
  ///
  /// \param atMin The axis at the minimum value end.
  /// \param atMax The axis at the maximum value end.
  void setNeigbors(const vtkQtChartAxis *atMin, const vtkQtChartAxis *atMax);

  /// \brief
  ///   Sets the parallel axis.
  /// \param across The axis parallel to this one.
  void setParallelAxis(const vtkQtChartAxis *across);

  /// \brief
  ///   Sets the axis' contents space object.
  /// \param contents The new contents space object.
  void setContentsSpace(const vtkQtChartContentsSpace *contents);

  /// \brief
  ///   Sets whether or not data is available for the axis.
  ///
  /// This parameter is used when the best fit range is zero. If data
  /// is available, the axis generates labels around the data value.
  /// This is needed when the data points are the same value for an
  /// axis, such as a vertical or horizontal line on a line chart.
  ///
  /// \param available True if data is available.
  void setDataAvailable(bool available);

  /// \brief
  ///   Gets whether or not the axis labels are generated from the
  ///   view size.
  /// \return
  ///   True if the axis labels are generated from the view size.
  bool isBestFitGenerated() const;

  /// \brief
  ///   Sets whether or not the axis labels are generated from the
  ///   view size.
  /// \param on True if the axis labels should be generated.
  void setBestFitGenerated(bool on);

  /// \brief
  ///   Gets the value range used when generating the axis labels.
  /// \param min Used to return the minimum value.
  /// \param max Used to return the maximum value.
  void getBestFitRange(QVariant &min, QVariant &max) const;

  /// \brief
  ///   Sets the value range used when generating the axis labels.
  /// \param min The minimum value.
  /// \param max The maximum value.
  void setBestFitRange(const QVariant &min, const QVariant &max);

  /// \brief
  ///   Gets whether or not range padding is used.
  ///
  /// This setting only affects the best-fit layout. When range
  /// padding is used, the axis makes sure there is space between
  /// the best-fit range and the actual minimum and maximum labels.
  ///
  /// \return
  ///   True if range padding is used.
  bool isRangePaddingUsed() const;

  /// \brief
  ///   Sets whether or not range padding is used.
  /// \param padRange True if range padding should be used.
  void setRangePaddingUsed(bool padRange);

  /// \brief
  ///   Gets whether or not the range is expanded to zero.
  ///
  /// This setting only affects the best-fit layout. If the range
  /// does not include zero, the range is expanded to include it.
  /// This is used by the bar chart to make sure the minimum bar has
  /// some length to it.
  ///
  /// \return
  ///   True if the range is expanded to zero.
  bool isExpansionToZeroUsed() const;

  /// \brief
  ///   Sets whether or not the range is expanded to zero.
  /// \param expand True if the range should be expanded to zero.
  void setExpansionToZeroUsed(bool expand);

  /// \brief
  ///   Gets whether or not extra space is added around the axis tick
  ///   marks.
  ///
  /// This setting does not affect the best-fit layout. Extra space is
  /// added before the minimum and after the maximum. This is useful
  /// for chart objects that expand around the tick mark like the bar
  /// in a bar chart.
  ///
  /// \return
  ///   True if extra space is added around the axis tick marks.
  bool isExtraSpaceUsed() const;

  /// \brief
  ///   Sets whether or not extra space is added around the axis tick
  ///   marks.
  /// \param addSpace True if space should be added around the axis
  ///   tick marks.
  void setExtraSpaceUsed(bool addSpace);

  /// \brief
  ///   Gets whether or not the space for the axis is too small.
  /// \return
  ///   True if the space for the axis is too small.
  bool isSpaceTooSmall() const;

  /// \brief
  ///   Sets whether or not the space for the axis is too small.
  /// \param tooSmall True if the space for the axis is too small.
  void setSpaceTooSmall(bool tooSmall);
  //@}

  /// \name Drawing Parameters
  //@{
  /// \brief
  ///   Gets the chart axis drawing options.
  /// \return
  ///   A pointer to the chart axis drawing options.
  vtkQtChartAxisOptions *getOptions() const {return this->Options;}

  /// \brief
  ///   Sets the chart axis drawing options.
  ///
  /// This method sets all the axis options at once, which can prevent
  /// unnecessary view updates.
  ///
  /// \param options The new axis drawing options.
  void setOptions(const vtkQtChartAxisOptions &options);
  //@}

  /// \name Display Methods
  //@{
  /// \brief
  ///   Used to layout the chart axis.
  ///
  /// This method must be called before the axis can be drawn.
  ///
  /// \param area The total chart area.
  void layoutAxis(const QRectF &area);

  /// \brief
  ///   Used to adjust the bounding width for the axis.
  ///
  /// This method only affects left and right locations. It is called
  /// after the neighboring axes have been layed out. The neighboring
  /// axes may need more space to display the labels.
  void adjustAxisLayout();

  /// \brief
  ///   Gets the space needed for the axis.
  ///
  /// If the axis is horizontal, the space needed is the preferred
  /// height. If the axis is vertical, the space needed is the
  /// preferred width. The preferred height can be obtained at any
  /// time. The preferred width is only valid after a layout.
  ///
  /// \return
  ///   The space needed for the axis.
  float getPreferredSpace() const;

  /// \brief
  ///   Gets the axis label font height.
  /// \return
  ///   The axis label font height.
  float getFontHeight() const;

  /// \brief
  ///   Gets the maximum label width.
  ///
  /// The maximum label width is only valid after a layout.
  ///
  /// \return
  ///   The maximum label width.
  float getMaxLabelWidth() const;

  /// \brief
  ///   Gets the length of the axis label tick marks.
  /// \return
  ///   The length of the axis label tick marks.
  float getTickLength() const;

  /// \brief
  ///   Gets the length of the small axis label tick marks.
  /// \return
  ///   The length of the small axis label tick marks.
  float getSmallTickLength() const;

  /// \brief
  ///   Gets the distance between the axis label and the tick mark.
  /// return
  ///   The distance between the axis label and the tick mark.
  float getTickLabelSpacing() const;

  /// \brief
  ///   Gets whether or not logarithmic scale can be used.
  /// \return
  ///   True if logarithmic scale can be used.
  bool isLogScaleAvailable() const;

  /// \brief
  ///   Paints the chart axis using the given painter.
  /// \param painter The painter to use.
  /// \param option The area to paint and the level of detail.
  /// \param widget The widget being painted.
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
      QWidget *widget=0);
  //@}

  /// \name Location Methods
  //@{
  virtual QRectF boundingRect() const;

  /// \brief
  ///   Gets the bounding rectangle for the axis.
  ///
  /// The bounding box returned is only valid after a layout. The
  /// rectangle is in chart coordinates.
  ///
  /// \return
  ///   The bounding rectangle for the axis.
  QRectF getBounds() const;

  /// \brief
  ///   Gets wether or not the given label tickmark is visible.
  /// \param index The index of the label.
  /// \return
  ///   True if the given label tickmark is visible.
  bool isLabelTickVisible(int index) const;

  /// \brief
  ///   Gets the location of the given label.
  /// \param index The index of the label.
  /// \return
  ///   The location of the given label.
  float getLabelLocation(int index) const;

  /// \brief
  ///   Gets the current axis domain.
  /// \return
  ///   The current axis domain.
  AxisDomain getAxisDomain() const;

  /// \brief
  ///   Gets whether or not the given value is in the axis domain.
  /// \param value The value to test.
  /// \return
  ///   True if the given value is in the axis domain.
  bool isValueInDomain(const QVariant &value) const;

  /// \brief
  ///   Gets the pixel location for the given value.
  ///
  /// The pixel location relates to the orientation of the axis.
  ///
  /// \param value The value to map.
  /// \return
  ///   The pixel location for the given value.
  float getPixel(const QVariant &value) const;

  /// \brief
  ///   Gets the pixel location for zero.
  ///
  /// For int and double domains, the pixel location returned is
  /// limited by the axis pixel range. For non-numeric domains, the
  /// axis minimum pixel location is returned.
  ///
  /// \return
  ///   The pixel location for zero.
  float getZeroPixel() const;
  //@}

  /// \brief
  ///   Gets whether or not logarithmic scale can be used for the
  ///   given range.
  /// \param min The range minimum.
  /// \param max The range maximum.
  /// \return
  ///   True if logarithmic scale can be used for the given range.
  static bool isLogScaleValid(const QVariant &min, const QVariant &max);

signals:
  /// Emitted when the axis needs to be layed out again.
  void layoutNeeded();

  /// \brief
  ///   Emitted when the pixel-value scale changes.
  ///
  /// The pixel-value scale is only modified inside the axis layout
  /// method. Charts using this signal should never emit
  /// \c layoutNeeded when responding to this signal. Instead, set a
  /// flag to use when the chart layout method is called.
  void pixelScaleChanged();

public slots:
  /// Resets the chart axis view.
  void reset();

  /// \brief
  ///   Sets the contents offset for the axis.
  /// \param offset The new contents offset.
  void setOffset(float offset);

private slots:
  /// Updates the layout for the new font.
  void handleFontChange();

  /// Updates the labels with the new presentation.
  void handlePresentationChange();

  /// Updates the axis and label colors.
  void handleColorChange();

  /// Updates the layout for the new axis scale.
  void handleAxisScaleChange();

  /// \brief
  ///   Adds the new label in the given location.
  /// \param index Where to insert the new label.
  void insertLabel(int index);

  /// \brief
  ///   Cleans up the view data for the given index.
  /// \param index The label being removed.
  void startLabelRemoval(int index);

  /// \brief
  ///   Finishes the label removal by requesting a relayout.
  /// \param index The label being removed.
  void finishLabelRemoval(int index);

private:
  /// \brief
  ///   Get the estimated maximum label width.
  ///
  /// This method is used when using a best-fit layout. The label
  /// width is estimated using the min and max values.
  ///
  /// \param minimum The minimum value.
  /// \param maximum The maximum value.
  /// \return
  ///   The estimated maximum label width.
  float getLabelWidthGuess(const QVariant &minimum,
      const QVariant &maximum) const;

  /// \brief
  ///   Generates labels for a linear, best-fit layout.
  /// \param contents The axis contents area.
  void generateLabels(const QRectF &contents);

  /// \brief
  ///   Generates labels for a logarithmic, best-fit layout.
  /// \param contents The axis contents area.
  void generateLogLabels(const QRectF &contents);

public:
  static const double MinLogValue;  ///< Stores the log scale minimum.

private:
  vtkQtChartAxisInternal *Internal;    ///< Stores the view data.
  vtkQtChartAxisOptions *Options;      ///< Stores the drawing options.
  vtkQtChartAxisModel *Model;          ///< Stores the list of labels.
  const vtkQtChartAxis *AtMin;         ///< Stores the axis at the min.
  const vtkQtChartAxis *AtMax;         ///< Stores the axis at the max.
  const vtkQtChartAxis *Across;        ///< Stores the parallel axis.
  const vtkQtChartContentsSpace *Zoom; ///< Stores the contents space.
  AxisLocation Location;               ///< Stores the axis location.
};

#endif
