/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLineChart.h

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

/// \file vtkQtLineChart.h
/// \date February 14, 2008

#ifndef _vtkQtChartLineLayer_h
#define _vtkQtChartLineLayer_h

#include "vtkQtChartExport.h"
#include "vtkQtChartSeriesLayer.h"

class vtkQtLineChartInternal;
class vtkQtLineChartOptions;


/// \class vtkQtLineChart
/// \brief
///   The vtkQtLineChart class is used to display a line chart.
class VTKQTCHART_EXPORT vtkQtLineChart : public vtkQtChartSeriesLayer
{
  Q_OBJECT

public:
  enum {Type = vtkQtChart_LineChartType};

public:
  vtkQtLineChart();
  virtual ~vtkQtLineChart();

  /// \name Setup Methods
  //@{
  virtual void setChartArea(vtkQtChartArea *area);

  virtual void setModel(vtkQtChartSeriesModel *model);
  //@}

  /// \name Drawing Parameters
  //@{
  /// \brief
  ///   Gets the line chart drawing options.
  /// \return
  ///   A pointer to the line chart drawing options.
  vtkQtLineChartOptions *getOptions() const {return this->Options;}

  /// \brief
  ///   Sets the line chart drawing options.
  ///
  /// This method sets all the options at once, which can prevent
  /// unnecessary view updates.
  ///
  /// \param options The new line chart drawing options.
  void setOptions(const vtkQtLineChartOptions &options);

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
  /// The chart search trees are not updated while the chart is in an
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
      QWidget *widget);
  //@}

public slots:
  /// \brief
  ///   Refreshes the line chart data from the model.
  ///
  /// The currently displayed data is cleaned up. If a model is set,
  /// it is used to populate the line chart.
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

  /// Called when this layer fires the layoutNeeded() signal is fired.
  void handleLayoutNeeded();

private slots:
  /// \brief
  ///   Prepares the line chart for a series insertion.
  /// \param first The first series index to be added.
  /// \param last The last series index to be added.
  void prepareSeriesInsert(int first, int last);

  /// \brief
  ///   Inserts line chart series at the given indexes.
  /// \param first The first series index to be added.
  /// \param last The last series index to be added.
  void insertSeries(int first, int last);

  /// \brief
  ///   Removes the specified series from the chart.
  /// \param first The first series index to be removed.
  /// \param last The last series index to be removed.
  void startSeriesRemoval(int first, int last);

  /// \brief
  ///   Finishes removing the series by updating the layout.
  /// \param first The first series index to be removed.
  /// \param last The last series index to be removed.
  void finishSeriesRemoval(int first, int last);

  /// \brief
  ///   Changes the series visibility.
  ///
  /// The signal sender is used to determine which series has changed.
  ///
  /// \param visible True if the series should be shown.
  void handleSeriesVisibilityChange(
    vtkQtChartSeriesOptions* options, bool visible);

  /// \brief
  ///   Changes the series axes corner.
  ///
  /// Both of the domains are updated for the change.
  ///
  /// \param corner The new axes corner.
  /// \param previous The previous axes corner.
  void handleSeriesAxesCornerChange(
    vtkQtChartSeriesOptions* options, int corner, int previous);

  /// Changes the series point marker.
  void handleSeriesPointMarkerChange(vtkQtChartSeriesOptions*);

  /// \brief
  ///   Called to update the highlights.
  ///
  /// The update request is ignored if the model is being changed.
  void updateHighlights();

private:
  /// \brief
  ///   Adds the domain for the given series to the given domain.
  /// \param series The series index.
  /// \param corner Which domain set to update.
  /// \param seriesGroup Used to return the domain group.
  /// \return
  ///   True if the domain was modified.
  bool addSeriesDomain(int series, vtkQtChartLayer::AxesCorner corner,
      int *seriesGroup);

  /// \brief
  ///   Calculates the domain for the given series group.
  /// \param seriesGroup The series group index.
  /// \param corner Which domain set to update.
  void calculateDomain(int seriesGroup, vtkQtChartLayer::AxesCorner corner);

  /// Builds the search trees for the line chart.
  void buildTree();

private:
  vtkQtLineChartInternal *Internal; ///< Stores the view data.
  vtkQtLineChartOptions *Options;   ///< Stores the drawing options.
  bool InModelChange;               ///< Used for selection changes.
  bool BuildNeeded;                 ///< Used when resizing interactively.
};

#endif
