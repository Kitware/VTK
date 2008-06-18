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

class vtkQtChartPointLocator;
class vtkQtLineChartInternal;
class vtkQtLineChartOptions;
class vtkQtLineChartSeriesOptions;


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

  void setPointLocator(vtkQtChartPointLocator *locator);
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

  vtkQtLineChartSeriesOptions *getLineSeriesOptions(int series) const;
  //@}

  /// \name Layout Methods
  //@{
  virtual void getLayerDomain(vtkQtChartLayerDomain &domain) const;

  /// \brief
  ///   Used to layout the chart layer.
  /// \param area The area the chart should occupy.
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
  virtual int type() const {return vtkQtLineChart::Type;}
  virtual QRectF boundingRect() const;
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
      QWidget *widget);
  //@}

public slots:
  void reset();

protected:
  virtual vtkQtChartSeriesOptions *createOptions(QObject *parent);
  virtual void setupOptions(vtkQtChartSeriesOptions *options);

private slots:
  void prepareSeriesInsert(int first, int last);
  void insertSeries(int first, int last);
  void startSeriesRemoval(int first, int last);
  void finishSeriesRemoval(int first, int last);
  void handleSeriesVisibilityChange(bool visible);
  void handleSeriesAxesCornerChange(int corner, int previous);
  void handleSeriesPointVisibilityChange(bool visible);
  void handleSeriesPointMarkerChange();
  void handleSeriesPenChange(const QPen &pen);
  void handleSeriesBrushChange(const QBrush &brush);
  void updateHighlights();

private:
  void layoutHighlights();
  bool addSeriesDomain(int series, vtkQtChartLayer::AxesCorner corner);
  void calculateDomain(int seriesGroup, vtkQtChartLayer::AxesCorner corner);

private:
  vtkQtLineChartInternal *Internal; ///< Stores the view data.
  vtkQtLineChartOptions *Options;   ///< Stores the drawing options.
  bool InModelChange;               ///< Used for selection changes.
};

#endif
