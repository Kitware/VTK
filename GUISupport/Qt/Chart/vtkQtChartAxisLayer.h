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

  const vtkQtChartAxisDomainPriority &getAxisDomainPriority(
      vtkQtChartAxis::AxisLocation location) const;

  void setAxisDomainPriority(vtkQtChartAxis::AxisLocation location,
      const vtkQtChartAxisDomainPriority &priority);
  //@}

  virtual void layoutChart(const QRectF &area);

  virtual void setChartArea(vtkQtChartArea *area);

  QRectF getLayerBounds() const {return this->LayerBounds;}

  virtual QRectF boundingRect() const;
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
    QWidget *widget=0);

public slots:
  void handleChartRangeChange();
  void cancelChartRangeChange();

private:
  void setupAxesCorner();
  vtkQtChartAxis::AxisDomain getAxisDomain(
      vtkQtChartAxis::AxisLocation location) const;
  vtkQtChartLayer::AxesCorner getCorner(vtkQtChartAxis::AxisLocation first,
      vtkQtChartAxis::AxisLocation second) const;
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
};

#endif
