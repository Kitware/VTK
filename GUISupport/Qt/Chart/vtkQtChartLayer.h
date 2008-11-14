/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartLayer.h

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

/// \file vtkQtChartLayer.h
/// \date February 1, 2008

#ifndef _vtkQtChartLayer_h
#define _vtkQtChartLayer_h

#include "vtkQtChartExport.h"
#include <QObject>
#include <QGraphicsItem>

#include "vtkQtChartGraphicsItemTypes.h" // needed for enum

class vtkQtChartAxis;
class vtkQtChartLayerDomain;
class vtkQtChartArea;
class QPainter;


/// \class vtkQtChartLayer
/// \brief
///   The vtkQtChartLayer class is the base class for all chart
///   drawing layers.
class VTKQTCHART_EXPORT vtkQtChartLayer : public QObject, public QGraphicsItem
{
  Q_OBJECT

public:
  enum AxesCorner
    {
    BottomLeft = 0, ///< Uses the bottom and left axes.
    BottomRight,    ///< Uses the bottom and right axes.
    TopLeft,        ///< Uses the top and left axes.
    TopRight        ///< Uses the top and right axes.
    };

  enum {Type = vtkQtChart_LayerType};

public:
  vtkQtChartLayer();
  virtual ~vtkQtChartLayer() {}

  virtual int type() const {return vtkQtChartLayer::Type;}

  /// \brief
  ///   Gets the chart area containing this layer.
  /// \return
  ///   A pointer to the chart area.
  vtkQtChartArea *getChartArea() const {return this->ChartArea;}

  /// \brief
  ///   Sets the chart area that contains this layer.
  ///
  /// The chart area will call this method when the layer is added to
  /// it. The layer can overload this method to perform any setup it
  /// needs to.
  ///
  /// \param area The new chart area.
  virtual void setChartArea(vtkQtChartArea *area) {this->ChartArea = area;}

  /// \brief
  ///   Gets the chart layer's domain.
  ///
  /// The layer should use the \c mergeDomain method of the \c domain
  /// parameter to add its domains. The domains from all of the chart
  /// layers will be combined using the \c domain object.
  ///
  /// \param domain Used to return the chart layer's domain.
  virtual void getLayerDomain(vtkQtChartLayerDomain &domain) const;

  /// \brief
  ///   Used to layout the chart layer.
  ///
  /// Use the \c AxisDomain from the relevant axes to determine if
  /// the chart data can be displayed. The domain priority for the
  /// axes can override a layer's desired domain.
  ///
  /// \param area The area the chart should occupy.
  virtual void layoutChart(const QRectF &area)=0;

  /// \brief
  ///   Used to filter items while drawing.
  ///
  /// Items can be filtered in two ways. First, this method can
  /// return true to skip drawing the item. Second, the painter can
  /// be modified to clip the item. The default implementation simply
  /// returns false.
  ///
  /// \note
  ///   The painter is in scene coordinates when passed in. It will
  ///   be transformed to item coordinates after this call. To clip
  ///   in item coordinates, use the item's paint method.
  /// \param item One of the layer's child items.
  /// \param painter The painter used to draw the item.
  /// \return
  ///   True if the item should not be drawn.
  virtual bool drawItemFilter(QGraphicsItem *item, QPainter *painter);

  /// \brief
  ///   Gets the help text for the given location.
  ///
  /// When the chart receives a help event, the layers are searched,
  /// in order, for help text at the help event location. If a layer
  /// has an item at the location, it should return true.
  ///
  /// \param point The help location in scene coordinates.
  /// \param text Used to return the help text.
  /// \return
  ///   True if help text is found for the given location.
  virtual bool getHelpText(const QPointF &point, QString &text);

  /// Notifies the chart layer that a resize interaction has started.
  virtual void startInteractiveResize() {}

  /// Notifies the chart layer that a resize interaction has finished.
  virtual void finishInteractiveResize() {}

signals:
  /// Emitted when the layer layout needs to be calculated.
  void layoutNeeded();

  /// \brief
  ///   Emitted when the axis range for the layer has changed.
  /// \note
  ///   This signal should be emitted before the \c layoutNeeded
  ///   signal to be effective.
  void rangeChanged();

protected:
  vtkQtChartArea *ChartArea; ///< Stores the containing chart area.
};

#endif
