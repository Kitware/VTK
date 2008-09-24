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
  enum {Type = vtkQtChart_BarChartType};

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

  /// \brief
  ///   Gets the bar chart series options.
  /// \param series The series index.
  /// \return
  ///   A pointer to the bar chart series options.
  vtkQtBarChartSeriesOptions *getBarSeriesOptions(int series) const;
  //@}

  /// \name Layout Methods
  //@{
  virtual void getLayerDomain(vtkQtChartLayerDomain &domain) const;

  virtual void layoutChart(const QRectF &area);

  virtual bool drawItemFilter(QGraphicsItem *item, QPainter *painter);

  virtual bool getHelpText(const QPointF &point, QString &text);
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

protected:
  /// \brief
  ///   Creates a new bar chart series options object.
  /// \param parent The parent object.
  /// \return
  ///   A pointer to the new bar chart series options object.
  virtual vtkQtChartSeriesOptions *createOptions(QObject *parent);

  /// \brief
  ///   Sets up the series options defaults.
  /// \param options The new series options object.
  virtual void setupOptions(vtkQtChartSeriesOptions *options);

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
  ///   Changes the series visibility.
  ///
  /// The signal sender is used to determine which series has changed.
  ///
  /// \param visible True if the series should be shown.
  void handleSeriesVisibilityChange(bool visible);

  /// \brief
  ///   Changes the series pen.
  /// \param pen The new series pen.
  void handleSeriesPenChange(const QPen &pen);

  /// \brief
  ///   Changes the series brush.
  /// \param brush The new series brush.
  void handleSeriesBrushChange(const QBrush &brush);

  /// \brief
  ///   Called to layout the highlights.
  ///
  /// The layout request is ignored if the model is being changed.
  void updateHighlights();

private:
  /// Called to layout the highlights.
  void layoutHighlights();

  /// \brief
  ///   Adds the domain for the given series to the current domain.
  /// \param series The series index.
  /// \return
  ///   True if the domain was modified.
  bool addSeriesDomain(int series);

  /// \brief
  ///   Calculates the domain for the given series group.
  /// \param seriesGroup The series group index.
  void calculateDomain(int seriesGroup);

private:
  vtkQtBarChartInternal *Internal; ///< Stores the bar series.
  vtkQtBarChartOptions *Options;   ///< Stores the drawing options.
  bool InModelChange;              ///< Used for selection changes.
};

#endif
