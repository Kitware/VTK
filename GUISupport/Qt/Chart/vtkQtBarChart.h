/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtBarChart.h

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

/// \file vtkQtBarChart.h
/// \date February 15, 2008

#ifndef _vtkQtBarChart_h
#define _vtkQtBarChart_h

#include "vtkQtChartExport.h"
#include "vtkQtChartSeriesLayer.h"

class vtkQtBarChartInternal;
class vtkQtBarChartOptions;
class vtkQtBarChartSeriesOptions;


/// \class vtkQtBarChart
/// \brief
///   The vtkQtBarChart class is used to display a bar chart.
class VTKQTCHART_EXPORT vtkQtBarChart : public vtkQtChartSeriesLayer
{
  Q_OBJECT

public:
  vtkQtBarChart();
  virtual ~vtkQtBarChart();

  /// \name Setup Methods
  //@{
  virtual void setChartArea(vtkQtChartArea *area);

  virtual void setModel(vtkQtChartSeriesModel *model);
  //@}

  /// \name Drawing Parameters
  //@{
  /// \brief
  ///   Gets the bar chart drawing options.
  /// \return
  ///   A pointer to the bar chart drawing options.
  vtkQtBarChartOptions *getOptions() const {return this->Options;}

  /// \brief
  ///   Sets the bar chart drawing options.
  ///
  /// This method sets all the options at once, which can prevent
  /// unnecessary view updates.
  ///
  /// \param options The new bar chart drawing options.
  void setOptions(const vtkQtBarChartOptions &options);

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
  /// The chart bar tree is not updated while the chart is in an
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
  ///   Refreshes the bar chart data from the model.
  ///
  /// The currently displayed data is cleaned up. If a model is set,
  /// it is used to populate the bar chart.
  void reset();

private slots:
  /// \brief
  ///   Prepares the bar chart for a series insertion.
  /// \param first The first series index to be added.
  /// \param last The last series index to be added.
  void prepareSeriesInsert(int first, int last);

  /// \brief
  ///   Inserts bar chart series at the given indexes.
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

  /// Changes the bar outline style.
  void handleOutlineChange();

  /// \brief
  ///   Called to set up the highlights.
  ///
  /// The set up request is ignored if the model is being changed.
  void updateHighlights();

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

private:
  /// \brief
  ///   Changes the series visibility.
  ///
  /// The signal sender is used to determine which series has changed.
  /// \param options The options that changed.
  /// \param visible True if the series should be shown.
  void handleSeriesVisibilityChange(vtkQtChartSeriesOptions* options, bool visible);

  /// \brief
  ///   Adds the domain for the given series to the current domain.
  /// \param series The series index.
  /// \param seriesGroup Used to return the series group index.
  /// \return
  ///   True if the domain was modified.
  bool addSeriesDomain(int series, int &seriesGroup);

  /// \brief
  ///   Calculates the domain for the given series group.
  /// \param seriesGroup The series group index.
  void calculateDomain(int seriesGroup);

  /// \brief
  ///   Creates an ordered list of series bars.
  /// \param seriesGroup The series group index.
  void createBarList(int seriesGroup);

  /// \brief
  ///   Builds the bar tree for the given series group.
  /// \param seriesGroup The series group index.
  void buildBarTree(int seriesGroup);

private:
  vtkQtBarChartInternal *Internal; ///< Stores the bar series.
  vtkQtBarChartOptions *Options;   ///< Stores the drawing options.
  bool InModelChange;              ///< Used for selection changes.
  bool BuildNeeded;                ///< Used when resizing interactively.

private:
  vtkQtBarChart(const vtkQtBarChart &);
  vtkQtBarChart &operator=(const vtkQtBarChart &);
};

#endif
