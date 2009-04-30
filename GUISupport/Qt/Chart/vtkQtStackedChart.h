/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStackedChart.h

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

/// \file vtkQtStackedChart.h
/// \date February 27, 2008

#ifndef _vtkQtChartStackedLayer_h
#define _vtkQtChartStackedLayer_h

#include "vtkQtChartExport.h"
#include "vtkQtChartSeriesLayer.h"

class vtkQtStackedChartInternal;
class vtkQtStackedChartOptions;


/// \class vtkQtStackedChart
/// \brief
///   The vtkQtStackedChart class is used to display a stacked chart.
class VTKQTCHART_EXPORT vtkQtStackedChart : public vtkQtChartSeriesLayer
{
  Q_OBJECT

public:
  vtkQtStackedChart();
  virtual ~vtkQtStackedChart();

  /// \name Setup Methods
  //@{
  virtual void setChartArea(vtkQtChartArea *area);

  virtual void setModel(vtkQtChartSeriesModel *model);
  //@}

  /// \name Drawing Parameters
  //@{
  /// \brief
  ///   Gets the stacked chart drawing options.
  /// \return
  ///   A pointer to the stacked chart drawing options.
  vtkQtStackedChartOptions *getOptions() const {return this->Options;}

  /// \brief
  ///   Sets the stacked chart drawing options.
  ///
  /// This method sets all the options at once, which can prevent
  /// unnecessary view updates.
  ///
  /// \param options The new stacked chart drawing options.
  void setOptions(const vtkQtStackedChartOptions &options);

  virtual QPixmap getSeriesIcon(int series) const;
  //@}

  /// \name Layout Methods
  //@{
  virtual void getLayerDomain(vtkQtChartLayerDomain &domain) const;

  virtual void layoutChart(const QRectF &area);

  virtual bool getHelpText(const QPointF &point, QString &text);

  /// \brief
  ///   Notifies the chart layer that a resize interaction has finished.
  ///
  /// The chart quad tree is not updated while the chart is in an
  /// interactive state. It is updated in this method if needed.
  virtual void finishInteractiveResize();
  //@}

  /// \name Selection Methods
  //@{
  virtual void getSeriesAt(const QPointF &point,
      vtkQtChartSeriesSelection &selection) const;

  virtual void getPointsAt(const QPointF &point,
      vtkQtChartSeriesSelection &selection) const;

  virtual void getSeriesIn(const QRectF &area,
      vtkQtChartSeriesSelection &selection) const;

  virtual void getPointsIn(const QRectF &area,
      vtkQtChartSeriesSelection &selection) const;
  //@}

  /// \name QGraphicsItem Methods
  //@{
  virtual QRectF boundingRect() const;
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
    QWidget *widget=0);
  //@}

public slots:
  /// \brief
  ///   Refreshes the stacked chart data from the model.
  ///
  /// The currently displayed data is cleaned up. If a model is set,
  /// it is used to populate the stacked chart.
  void reset();

protected slots:
  /// \brief
  ///   Called when any of the series options are changed.
  ///  Default implementation fires the modelSeriesChanged() signal.
  /// \param options The options that fired the dataChanged() signal.
  /// \param type Type of the option that was changed.
  /// \param newValue The new value for the option.
  /// \param oldValue The previous value for the option, if any.
  virtual void handleOptionsChanged(vtkQtChartSeriesOptions*,
    int type, const QVariant& newvalue, const QVariant& oldvalue);

private slots:
  /// \brief
  ///   Prepares the stacked chart for a series insertion.
  /// \param first The first series index to be added.
  /// \param last The last series index to be added.
  void prepareSeriesInsert(int first, int last);

  /// \brief
  ///   Inserts stacked chart series at the given indexes.
  /// \param first The first series index to be added.
  /// \param last The last series index to be added.
  void insertSeries(int first, int last);

  /// \brief
  ///   Removes the specified series from the chart.
  /// \param first The first series index to be removed.
  /// \param last The last series index to be removed.
  void startSeriesRemoval(int first, int last);

  /// \brief
  ///   Finishes the domain changes after removing the series.
  /// \param first The first series index to be removed.
  /// \param last The last series index to be removed.
  void finishSeriesRemoval(int first, int last);

  /// Requests a chart layout when the axis corner option changes.
  void handleAxesCornerChange();

  /// Updates the chart when the sumation type has changed.
  void handleSumationChange();

  /// Updates the chart when the gradient option has changed.
  void handleGradientChange();

  /// \brief
  ///   Called to layout the highlights.
  ///
  /// The layout request is ignored if the model is being changed.
  void updateHighlights();

  /// Called by the animation timeline to animate hiding a series.
  void seriesVisibilityAnimate(qreal time);

  /// Called by the animation timeline when animation is finished.
  void seriesVisibilityAnimateFinished();

private:
  /// \brief
  ///   Changes the series visibility.
  ///
  /// The signal sender is used to determine which series has changed.
  ///
  /// \param options Options that changed.
  /// \param visible True if the series should be shown.
  void handleSeriesVisibilityChange(
    vtkQtChartSeriesOptions* options, bool visible);

  /// Called to layout the highlights.
  void layoutHighlights();

  /// \brief
  ///   Adds the domain for the given series to the current domain.
  /// \param series The series index
  /// \param seriesGroup Used to return the domain group.
  void addSeriesDomain(int series, int *seriesGroup);

  /// \brief
  ///   Updates the series table index map.
  /// \param seriesGroup The domain group to update.
  void updateItemMap(int seriesGroup);

  /// \brief
  ///   Creates the table for the given series domain group.
  /// \param seriesGroup The domain group index.
  void createTable(int seriesGroup);

  /// \brief
  ///   Normalizes the table for the given series domain group.
  /// \param seriesGroup The domain group index.
  void normalizeTable(int seriesGroup);

  /// \brief
  ///   Calculates the x-axis domain for the given domain group.
  /// \param seriesGroup The domain group index.
  void calculateXDomain(int seriesGroup);

  /// \brief
  ///   Calculates the y-axis domain for the given domain group.
  /// \param seriesGroup The domain group index.
  void calculateYDomain(int seriesGroup);

  /// \brief
  ///   Creates an ordered table of series quadrilaterals.
  /// \param seriesGroup The domain group index.
  void createQuadTable(int seriesGroup);

  /// \brief
  ///   Builds the quad tree for the given domain group.
  /// \param seriesGroup The domain group index.
  void buildQuadTree(int seriesGroup);

private:
  vtkQtStackedChartInternal *Internal; ///< Stores the series.
  vtkQtStackedChartOptions *Options;   ///< Stores the drawing options.
  bool InModelChange;                  ///< Used for selection changes.
  bool BuildNeeded;                    ///< Used when resizing interactively.
};

#endif
