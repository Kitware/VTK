/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxisLayer.h

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

/// \file vtkQtChartAxisLayer.h
/// \date February 1, 2008

#ifndef _vtkQtChartAxisLayer_h
#define _vtkQtChartAxisLayer_h

#include "vtkQtChartExport.h"
#include "vtkQtChartLayer.h"
#include "vtkQtChartAxis.h" // Needed for enum

class vtkQtChartAxisDomain;
class vtkQtChartAxisDomainPriority;
class vtkQtChartAxisLayerItem;
class QFont;
class QGraphicsRectItem;


/// \class vtkQtChartAxisLayer
/// \brief
///   The vtkQtChartAxisLayer class is used to display the chart axes.
class VTKQTCHART_EXPORT vtkQtChartAxisLayer : public vtkQtChartLayer
{
  Q_OBJECT

public:
  enum AxisBehavior
    {
    ChartSelect = 0, ///< The axis labels are determined by the charts.
    BestFit,         ///< The axis labels are determined by space.
    FixedInterval    ///< The axis labels are fixed.
    };

  enum {Type = vtkQtChart_AxisLayerType};

public:
  vtkQtChartAxisLayer();
  virtual ~vtkQtChartAxisLayer();

  /// \name Axis Methods
  //@{
  /// \brief
  ///   Gets the axis in the specified location.
  /// \param location The location of the axis.
  /// \return
  ///   A pointer to the specified axis.
  vtkQtChartAxis *getAxis(vtkQtChartAxis::AxisLocation location) const;

  /// \brief
  ///   Gets the horizontal axis in the specified pair.
  /// \param axes The pair of chart axes.
  /// \return
  ///   A pointer to the specified axis.
  vtkQtChartAxis *getHorizontalAxis(vtkQtChartLayer::AxesCorner axes) const;

  /// \brief
  ///   Gets the vertical axis in the specified pair.
  /// \param axes The pair of chart axes.
  /// \return
  ///   A pointer to the specified axis.
  vtkQtChartAxis *getVerticalAxis(vtkQtChartLayer::AxesCorner axes) const;

  /// \brief
  ///   Gets the layout behavior for the specified axis.
  /// \param location The location of the axis.
  /// \return
  ///   The layout behavior for the specified axis.
  AxisBehavior getAxisBehavior(vtkQtChartAxis::AxisLocation location) const;

  /// \brief
  ///   Sets the layout behavior for the specified axis.
  /// \param location The location of the axis.
  /// \param behavior The new layout behavior.
  void setAxisBehavior(vtkQtChartAxis::AxisLocation location,
      AxisBehavior behavior);

  /// \brief
  ///   Gets the domain priority order for the given axis.
  /// \param location The location of the axis.
  /// \return
  ///   A reference to the domain priority order.
  const vtkQtChartAxisDomainPriority &getAxisDomainPriority(
      vtkQtChartAxis::AxisLocation location) const;

  /// \brief
  ///   Sets the domain priority order for the given axis.
  /// \param location The location of the axis.
  /// \param priority The new domain priority order.
  void setAxisDomainPriority(vtkQtChartAxis::AxisLocation location,
      const vtkQtChartAxisDomainPriority &priority);
  //@}

  /// \brief
  ///   Used to layout the chart axes.
  ///
  /// The \c area passed to this method is the total chart area. The
  /// axis layer uses the whole area to layout the axes. Once the
  /// layout is complete, the space inside the axes is passed to the
  /// other chart layers.
  ///
  /// \param area The area the axes should occupy.
  /// \sa
  ///   vtkQtChartAxisLayer::getLayerBounds()
  virtual void layoutChart(const QRectF &area);

  /// \brief
  ///   Sets the chart area that contains this layer.
  ///
  /// The contents space object for the axes is set to the one used
  /// by the chart area.
  ///
  /// \param area The new chart area.
  virtual void setChartArea(vtkQtChartArea *area);

  /// \brief
  ///   Gets the area inside the chart axes.
  /// \return
  ///   The area inside the chart axes.
  QRectF getLayerBounds() const {return this->LayerBounds;}

  virtual QRectF boundingRect() const;
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
    QWidget *widget=0);

public slots:
  /// Sets a flag to gather the chart domains during layout.
  void handleChartRangeChange();

  /// Clears the flag to gather the chart domains during layout.
  void cancelChartRangeChange();

private:
  /// Sets up the axis objects for the chart.
  void setupAxesCorner();

  /// \brief
  ///   Gets the domain type for the given axis.
  /// \param location The location of the axis.
  /// \return
  ///   The axis domain type.
  vtkQtChartAxis::AxisDomain getAxisDomain(
      vtkQtChartAxis::AxisLocation location) const;

  /// \brief
  ///   Gets the corner for the given set of axes.
  /// \param first The location of the first axis.
  /// \param second The location of the second axis.
  /// \return
  ///   The corner for the given set of axes.
  vtkQtChartLayer::AxesCorner getCorner(vtkQtChartAxis::AxisLocation first,
      vtkQtChartAxis::AxisLocation second) const;

  /// \brief
  ///   Finds the domain for the axis based on the chart domains.
  /// \param axis The location of the axis.
  /// \param neighbor The location of the neighboring axis.
  /// \param neighborDomain The domain type of the neighboring axis.
  /// \param layerDomain The chart layer domains.
  /// \param axisDomain Used to return the axis domain.
  void findAxisDomain(vtkQtChartAxis::AxisLocation axis,
      vtkQtChartAxis::AxisLocation neighbor,
      vtkQtChartAxis::AxisDomain neighborDomain,
      const vtkQtChartLayerDomain &layerDomain,
      vtkQtChartAxisDomain &axisDomain) const;

private:
  QRectF LayerBounds;                 ///< Stores the layer bounds.
  QGraphicsRectItem *Border;          ///< Draws the layer boundary.
  vtkQtChartAxis *Axis[4];            ///< Stores the axis objects.
  vtkQtChartAxisLayerItem *Option[4]; ///< Stores the axis behaviors.
  bool RangeChanged;                  ///< True if the range has changed.

private:
  vtkQtChartAxisLayer(const vtkQtChartAxisLayer &);
  vtkQtChartAxisLayer &operator=(const vtkQtChartAxisLayer &);
};

#endif
